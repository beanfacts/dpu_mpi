#include "dpucommon.h"
#include "server_opts.h"
#include "get_ip.h"

/* MPI stuff */
int rank, worldsize;

uint64_t pagesize;

// Completion status
typedef enum {
    RS_UNALLOC  = 0,        // Unallocated

    RS_READ     = 1,        // Allocated - running RDMA read
    RS_READOK   = 2,        // Read OK

    RS_MPI      = 3,        // Running MPI_Ialltoall
    RS_MPIOK    = 4,        // MPI Ialltoall completed

    RS_PUT      = 5,        // Putting data into host buffer
    RS_PUTOK    = 6,        // Put completed - run completed

    RS_CPIN     = 7,        // Completed, but memory is still pinned.
    RS_FAIL     = 10        // Failed
} ReqCode;

// Struct to contain metadata about a pending MPI task
typedef struct {
    OffloadReq              req;
    struct ibv_mr           *bf_send_mr;
    uint64_t                send_alloc;
    uint64_t                send_used;
    uint64_t                send_remaining;
    struct ibv_mr           *bf_recv_mr;
    uint64_t                recv_alloc;
    uint64_t                recv_used;
    uint64_t                recv_remaining;
    MPI_Request             mpi_req;
    ReqCode                 status;
} OffloadJob;


// Struct to contain basic RDMA connection information
typedef struct {
    struct rdma_cm_id   *connid;
    struct ibv_mr       *send_mr;
    struct ibv_mr       *recv_mr;
    OffloadJob          *joblist;
    int                 joblist_len;
} RDMAContext;

OffloadJob *job_list;

/*
    Free all memory from completed or failed transfers.
    Returns 0 on success, 1 on error.
*/
int dpu_free_all_completed(OffloadJob *job, int job_length)
{
    int ret;
    for (int i = 0; i < job_length; i++)
    {
        if (job[i].status == RS_CPIN || job[i].status == RS_FAIL)
        {
            
            // Free send buffer
            if (job[i].bf_send_mr)
            {
                void *alloc_addr = job[i].bf_send_mr->addr;
                ret = rdma_dereg_mr(job[i].bf_send_mr);
                if (ret)
                {
                    fprintf(stderr, "RDMA send dereg failed: %s\n", strerror(errno));
                    return 1;
                }
                free(alloc_addr);
            }

            // Free recv buffer
            if (job[i].bf_recv_mr)
            {
                void *alloc_addr = job[i].bf_recv_mr->addr;
                ret = rdma_dereg_mr(job[i].bf_recv_mr);
                if (ret)
                {
                    fprintf(stderr, "RDMA recv dereg failed: %s\n", strerror(errno));
                    return 1;
                }
                free(alloc_addr);
            }
        }
    }
    return 0;
}

/*
    Check the job queue for completed jobs that still have memory
    pinned, and reuse the buffer if it's long enough.
*/
int dpu_reuse_mr(OffloadJob *job, int job_length, uint64_t send_alloc, uint64_t recv_alloc, OffloadJob *my_job)
{
    // Find a usable MR
    struct ibv_mr *send_mr      = NULL;
    struct ibv_mr *recv_mr      = NULL;
    uint64_t min_send           = -1;
    uint64_t min_recv           = -1;

    // Entry for list removal
    int send_idx                = -1;
    int send_idx_is_send        = -1;
    int recv_idx                = -1;
    int recv_idx_is_send        = -1;
    
    // Find a usable send region
    for (int i = 0; i < job_length; i++)
    {
        // Ignore entries not completed + pinned
        if (job[i].status != RS_CPIN)
        {
            continue;
        }
        
        // Check if we can reuse the old send MR for sendbuf
        if (job[i].bf_send_mr && job[i].send_alloc >= send_alloc && job[i].send_alloc < min_send)
        {
            send_mr     = job[i].bf_send_mr;
            min_send    = job[i].send_alloc;
            send_idx = i;
            send_idx_is_send = 1;
        }
        
        // Check if we can reuse the old recv MR for sendbuf
        if (job[i].bf_recv_mr && job[i].recv_alloc >= send_alloc && job[i].recv_alloc < min_send)
        {
            send_mr     = job[i].bf_recv_mr;
            min_send    = job[i].recv_alloc;
            send_idx = i;
            send_idx_is_send = 0;
        }
    }

    // Find a usable recv region
    for (int i = 0; i < job_length; i++)
    {
        // Ignore entries not completed + pinned
        if (job[i].status != RS_CPIN)
        {
            continue;
        }
        
        // Check if we can reuse the old send MR for recvbuf
        if (job[i].bf_send_mr && job[i].send_alloc >= recv_alloc && job[i].send_alloc < min_recv && job[i].bf_send_mr != send_mr)
        {
            recv_mr     = job[i].bf_send_mr;
            min_recv    = job[i].send_alloc;
            recv_idx = i;
            recv_idx_is_send = 1;
        }
        
        // Check if we can reuse the old recv MR for recvbuf
        if (job[i].bf_send_mr && job[i].recv_alloc >= recv_alloc && job[i].recv_alloc < min_recv && job[i].bf_recv_mr != send_mr)
        {
            recv_mr     = job[i].bf_recv_mr;
            min_recv    = job[i].recv_alloc;
            recv_idx = i;
            recv_idx_is_send = 0;
        }
    }

    // If we cannot find anything, we will free all the pinned memory.
    // This gives the best chance for malloc/reg_mr later.
    // This is not the best solution, but it will do for now
    if (!send_mr || !recv_mr)
    {
        //printf("Could not find usable memory region...\n");
        dpu_free_all_completed(job, job_length);
        return 1;
    }
    
    // Clear the old send MR entry
    if (send_idx >= 0)
    {
        if (send_idx_is_send)
        {
            job[send_idx].bf_send_mr        = NULL;
            job[send_idx].send_alloc        = 0;
            job[send_idx].send_remaining    = 0;
            job[send_idx].send_used         = 0;
        }
        else
        {
            job[send_idx].bf_recv_mr        = NULL;
            job[send_idx].recv_alloc        = 0;
            job[send_idx].recv_remaining    = 0;
            job[send_idx].recv_used         = 0;
        }
    }

    // Clear the old recv MR entry
    if (recv_idx >= 0)
    {
        if (recv_idx_is_send)
        {
            job[recv_idx].bf_send_mr        = NULL;
            job[recv_idx].send_alloc        = 0;
            job[recv_idx].send_remaining    = 0;
            job[recv_idx].send_used         = 0;
        }
        else
        {
            job[recv_idx].bf_recv_mr        = NULL;
            job[recv_idx].recv_alloc        = 0;
            job[recv_idx].recv_remaining    = 0;
            job[recv_idx].recv_used         = 0;
        }
    }

    // Free any items with all empty entries
    if (!job[recv_idx].bf_recv_mr && !job[recv_idx].bf_send_mr)
    {
        job[recv_idx].status = RS_UNALLOC;
    }
    if (!job[send_idx].bf_recv_mr && !job[send_idx].bf_send_mr)
    {
        job[send_idx].status = RS_UNALLOC;
    }

    // Put the MR data in my job entry
    my_job->bf_send_mr  = send_mr;
    my_job->send_alloc  = min_send;
    my_job->send_used   = send_alloc;
    my_job->bf_recv_mr  = recv_mr;
    my_job->recv_alloc  = min_recv;
    my_job->recv_used   = recv_alloc;
    return 0;
}

/*  Add an offloaded RDMA read to the queue.
    Order: [offload_read] -> offload_ialltoall -> offload_write
    0: OK; 1: User error; 2: Server error
*/
int offload_read(RDMAContext *ctx, OffloadJob *job)
{
    
    int ret;
    
    if (job->status != RS_UNALLOC)
    {
        fprintf(stderr, "Read offload called in wrong order! %d\n", job->status);
        return 1;
    }

    //printf("Starting read offload...\n");
    
    /* Calculate and allocate the memory the client requested */

    MPI_Datatype send_type = DPU_MPI_Type_Unpack(job->req.send_datatype);
    MPI_Datatype recv_type = DPU_MPI_Type_Unpack(job->req.recv_datatype);

    int send_elem_size, recv_elem_size;
    ret = MPI_Type_size(send_type, &send_elem_size);
    if (ret)
    {
        fprintf(stderr, "Invalid send type\n");
        goto out_user;
    }

    ret = MPI_Type_size(recv_type, &recv_elem_size);
    if (ret)
    {
        fprintf(stderr, "Invalid recv type\n");
        goto out_user;
    }
    
    job->recv_used = job->req.recv_elems * recv_elem_size * worldsize;
    job->send_used = job->req.send_elems * send_elem_size * worldsize;

    // Attempt to reuse an MR. If it failed, reallocate memory
    ret = dpu_reuse_mr(ctx->joblist, ctx->joblist_len, job->send_used, job->recv_used, job);
    if (ret)
    {
        //printf("Couldn't find a memory region... reallocating.\n");
        
        void *bf_sendbuf, *bf_recvbuf;
        ret = posix_memalign(&bf_sendbuf, pagesize, job->send_used);
        if (ret)
        {
            fprintf(stderr, "Failed to allocate page-aligned memory.\n");
            goto out_error;
        }

        ret = posix_memalign(&bf_recvbuf, pagesize, job->recv_used);
        if (ret)
        {
            fprintf(stderr, "Failed to allocate page-aligned memory.\n");
            goto out_error;
        }

        job->send_alloc = job->send_used;
        job->recv_alloc = job->recv_used;

        if (!bf_sendbuf || !bf_recvbuf)
        {
            fprintf(stderr, "Failed to allocate memory.\n");
            goto out_error;
        }
        job->bf_send_mr = rdma_reg_msgs(ctx->connid, bf_sendbuf, job->send_used);
        if (!job->bf_send_mr)
        {
            fprintf(stderr, "Failed to register memory 1.\n");
            goto out_error;
        }
        job->bf_recv_mr = rdma_reg_msgs(ctx->connid, bf_recvbuf, job->recv_used);
        if (!job->bf_recv_mr)
        {
            fprintf(stderr, "Failed to register memory 2.\n");
            return 1;
        }
    }

    //printf("-- Reading %lu bytes %lu [%d] -> %p \n", job->send_used, job->req.send_buf, job->req.send_rkey, job->bf_send_mr->addr);
    //printf("(%d) Elems: %d; Size: %d; World: %d => %lu\n", rank, job->req.send_elems, send_elem_size, worldsize, job->send_used);
    ret = rdma_post_read(ctx->connid, job, job->bf_send_mr->addr, job->send_used, job->bf_send_mr, 0, job->req.send_buf, job->req.send_rkey);
    if (ret < 0)
    {
        fprintf(stderr, "Could not post RDMA read. %s\n", strerror(errno));
        goto out_error;
    }

    //printf("Post read context: %p\n", job);

    job->status = RS_READ;
    return 0;

out_user:
    job->status = RS_FAIL;
    return 1;

out_error:
    job->status = RS_FAIL;
    return 2;

}


/*  Perform MPI_Ialltoall from the buffer we gathered earlier.
    Then we can wait on this using MPI_Test.
    Order: offload_read -> [offload_ialltoall] -> offload_write
*/
int offload_ialltoall(RDMAContext *ctx, OffloadJob *job)
{
    //printf("Performing ialltoall offload...\n");
    
    if (job->status != RS_READOK)
    {
        fprintf(stderr, "Ialltoall offload called in wrong order! %d\n", job->status);
        return 1;
    }

    // Unpack the datatype to be passed to MPI function.
    int ret;
    MPI_Datatype send_type = DPU_MPI_Type_Unpack(job->req.send_datatype);
    MPI_Datatype recv_type = DPU_MPI_Type_Unpack(job->req.recv_datatype);

    //printf("(%d) Stype: %p Rtype: %p Scnt: %d, Rcnt: %d, SMR: %p, RMR: %p, Req: %p\n", rank, send_type, recv_type, job->req.send_elems, job->req.recv_elems, job->bf_send_mr->addr, job->bf_recv_mr->addr, job->mpi_req);
    //printf("MPI_COMM_WORLD=%p\n", MPI_COMM_WORLD);
    
    // Now run MPI Ialltoall on BlueField
    ret = MPI_Ialltoall(    job->bf_send_mr->addr, job->req.send_elems, send_type,
                            job->bf_recv_mr->addr, job->req.recv_elems, recv_type,
                            MPI_COMM_WORLD, &job->mpi_req);


    //printf("----- Queued the request.\n");

    if (!job->mpi_req)
    {
        fprintf(stderr, "MPI Request is empty!\n");
        job->status = RS_FAIL;
        return 1;
    }
    
    if (ret)
    {
        fprintf(stderr, "MPI alltoall failed %d!\n", ret);
        job->status = RS_FAIL;
        return 1;
    }

    // Set state to MPI in progress
    job->status = RS_MPI;
    return ret;
}


/* 
    Perform the write back into host memory.
*/
int offload_write(RDMAContext *ctx, OffloadJob *job)
{

    //printf("Performing write offload...\n");
    
    if (job->status != RS_MPIOK)
    {
        fprintf(stderr, "offload_write() was called in the wrong order!\n");
        return 1;
    }

    int ret;

    // Determine amount to allocate for receive
    int recv_type_size;
    MPI_Datatype recv_type = DPU_MPI_Type_Unpack(job->req.recv_datatype);
    ret = MPI_Type_size(recv_type, &recv_type_size);
    if (ret)
    {
        fprintf(stderr, "Invalid type\n");
        goto out_user;
    }
    uint64_t recv_alloc = job->req.recv_elems * recv_type_size * worldsize;
    
    ret = rdma_ext_post_write_imm(  
            ctx->connid, job, job->bf_recv_mr->addr, 
            recv_alloc, job->bf_recv_mr, 0, job->req.recv_buf, 
            job->req.recv_rkey, job->req.cookie );
    if (ret)
    {
        fprintf(stderr, "Post write failed.\n");
        goto out_error;
    }

    //ret = rdma_post_write(ctx->connid, job, job->bf_recv_mr->addr, recv_alloc, job->bf_recv_mr, 0, job->req.recv_buf, job->req.recv_rkey);

    job->status = RS_PUT;
    return 0;

out_user:
    job->status = RS_FAIL;
    return 1;

out_error:
    job->status = RS_FAIL;
    return 2;

}


/*
    When a job changes state, the pointer of the job list entry
    can be passed in here to advance it.
    Returns 1 if job could not be advanced.
*/
int bf_advance(RDMAContext *ctx, OffloadJob *job)
{
    int ret;
    if (job->status == RS_READ)
    {
        //printf("Advancing to ialltoall\n");
        job->status = RS_READOK;
        int ret = offload_ialltoall(ctx, job);
        if (ret)
        {
            fprintf(stderr, "Advance failure!\n");
            return 1;
        }
        return 0;
    }
    else if (job->status == RS_PUT)
    {
        //printf("Cleaning up...\n");
        job->status = RS_CPIN;
        if (LAZY_UNPINNING == 0)
        {
            ret = dpu_free_all_completed(job, 1);
            if (ret)
            {
                fprintf(stderr, "Could not free item!!!\n");
                return 1;
            }
            memset(job, 0, sizeof(OffloadJob));
        }
        return 0;
    }
    return 1;
}


/*
    Poll items in the job queue. Because the main function already
    performs ibv_poll_cq for read/recv we only need to check MPI status
    and send cq.
*/
int bf_poll(RDMAContext *ctx, OffloadJob *job, int joblen)
{
    int ret, flag;
    struct ibv_wc wc;
    for (int i = 0; i < joblen; i++)
    {   
        if (job[i].status == RS_READ || job[i].status == RS_PUT)
        {
            // Check completion
            ret = ibv_poll_cq(ctx->connid->send_cq, 1, &wc);
            if (ret < 0)
            {
                fprintf(stderr, "poll CQ failed %d\n", ret);
                return -2;
            }
            else if (ret > 0)
            {
                if (wc.opcode == IBV_WC_RDMA_READ || wc.opcode == IBV_WC_RDMA_WRITE)
                {
                    ret = bf_advance(ctx, (OffloadJob *) wc.wr_id);
                    if (ret)
                    {
                        fprintf(stderr, "Job advancement failed.\n");
                        return -2;
                    }
                }
                else
                {
                    fprintf(stderr, "Warning: Unknown opcode %d\n", wc.opcode);
                    return -2;
                }
            }
        }
        
        if (job[i].status == RS_MPI)
        {
            ret = MPI_Test(&(job[i].mpi_req), &flag, MPI_STATUS_IGNORE);
            if (ret != MPI_SUCCESS)
            {
                fprintf(stderr, "MPI wait failed.\n");
                return -2;
            }
            if (flag)
            {
                job[i].status = RS_MPIOK;
                ret = offload_write(ctx, &job[i]);
                if (ret)
                {
                    fprintf(stderr, "Could not perform write offload\n");
                    return -2;
                }
            }
        }
    }
    return -1;
}

/*
    Main offload function.
*/
int offload_main(RDMAContext *ctx)
{
    int ret;
    struct ibv_wc wc;
    OffloadReq bf_req;
    int recv_status = 0;

    ctx->joblist = job_list;
    ctx->joblist_len = MAX_QUEUE;
    
    while (1)
    {
        
        // Post recv if pending
        if (recv_status == 0)
        {
            ret = rdma_post_recv(ctx->connid, (void *) 5, ctx->recv_mr->addr, pagesize, ctx->recv_mr);
            if (ret)
            {
                fprintf(stderr, "Failed to post receive!\n");
                return 1;
            }
            recv_status = 1;
        }

        // Check on the state of existing jobs
        ret = bf_poll(ctx, ctx->joblist, ctx->joblist_len);
        if (ret == -2)
        {
            fprintf(stderr, "Failed to perform poll.\n");
            return 1;
        }

        // Poll for a receive completion, meaning a new message
        // from the client.
        ret = ibv_poll_cq(ctx->connid->recv_cq, 1, &wc);
        if (ret < 0)
        {
            fprintf(stderr, "poll CQ failed %d\n", ret);
            return 1;
        }
        else if (ret > 0)
        {
            if (wc.status != IBV_WC_SUCCESS)
            {
                fprintf(stderr, "(%d) Transport Error: %s\n", rank, ibv_wc_status_str(wc.status));
                return 1;
            }

            //printf("Good cookie...\n");

            if (wc.opcode == IBV_WC_RECV)
            {
                recv_status = 2;
            }
            else
            {
                fprintf(stderr, "Warning: Unknown opcode %d\n", wc.opcode);
                return 1;
            }
        }

        // Got my data - offload time
        if (recv_status == 2)
        {
            if (wc.byte_len == 1)
            {
                //printf("Received disconnect...\n");
                ret = rdma_disconnect(ctx->connid);
                if (ret)
                {
                    fprintf(stderr, "Disconnect failed...\n");
                    return 1;
                }
                return 0;
            }
            
            // Deserialize the incoming data
            //printf("Recv %d bytes\n", wc.byte_len);
            pb_istream_t req_raw = pb_istream_from_buffer(ctx->recv_mr->addr, wc.byte_len);
            ret = (int) pb_decode(&req_raw, OffloadReq_fields, &bf_req);

            if (!ret)
            {
                fprintf(stderr, "Message decode error - not continuing\n");
                return 1;
            }

            // Clear the buffer for the next message
            memset(ctx->recv_mr->addr, 0, wc.byte_len);

            // Find free queue slot
            int j;
            for (j = 0; j < MAX_QUEUE; j++)
            {
                if (job_list[j].status == RS_UNALLOC)
                {
                    job_list[j].req = bf_req;
                    ret = offload_read(ctx, &job_list[j]);
                    if (ret)
                    {
                        fprintf(stderr, "Read offload failed.\n");
                        return 1;
                    }
                    break;
                }
            }

            // If the queue is full then undefined behaviour
            if (j >= MAX_QUEUE)
            {
                fprintf(stderr, "Error: Queue full!\n");
                return 1;
            }
            
            // Now that we copied everything we can receive new data
            recv_status = 0;
        }
    }
}


int main(int argc, char **argv)
{
    int ret;

    // Allocate space for jobs    
    job_list = calloc(1, sizeof(OffloadJob) * MAX_QUEUE);
    if (!job_list)
    {
        fprintf(stderr, "Could not allocate space for jobs\n");
        return 1;
    }
    
    // Initialize MPI
    ret = init_mpi(&argc, &argv, &rank, &worldsize);
    if (ret)
    {
        fprintf(stderr, "Error %d while trying to init MPI.\n", ret);
        return 1;
    }

    ret = test_mpi(rank);
    if (ret)
    {
        fprintf(stderr, "MPI Basic Check failed\n");
        return 1;
    }
    
    RDMAContext ctx;
    
    memset(job_list, 0, sizeof(job_list) * MAX_QUEUE);
    pagesize = sysconf(_SC_PAGESIZE);

    struct rdma_cm_id *sockid, *connid;
    char *listen_addr = "0.0.0.0";
    char *listen_port = "9999";

    
    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;

    // Decode listen addr and port
    printf("(%d) -> %s:%s\n", rank, find_addr("ib0_mlx5", -1), listen_port);
    //printf("Attempting to listen on %s:%s\n", listen_addr, listen_port);
    hints.ai_port_space = RDMA_PS_TCP;
    hints.ai_flags      = RAI_PASSIVE;
    ret = rdma_getaddrinfo(listen_addr, listen_port, &hints, &host_res);
    if (ret)
    {
        fprintf(stderr, "Could not resolve host.\n");
        goto out;
    }

    // Set up the endpoint
    new(struct ibv_qp_init_attr, init_attr);
    init_attr.cap.max_send_wr = 10;
    init_attr.cap.max_recv_wr = 10;
    init_attr.cap.max_send_sge = 10;
    init_attr.cap.max_recv_sge = 10;
    init_attr.sq_sig_all = 1;

    ret = rdma_create_ep(&sockid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create connection\n");
        goto out;
    }
    
    //printf("Created RDMA connection\n");

    // Listen for incoming connections - we only need to support
    // one connection when offloading so no need for backlog
    ret = rdma_listen(sockid, 0);
    if (ret)
    {
        fprintf(stderr, "Failed to listen on RDMA channel.\n");
        goto out;
    }

    // Allocate memory for buffers and client info
    //printf("Listening on channel %p\n", sockid);
    struct sockaddr_in  *client_id  = calloc(1, sizeof(struct sockaddr_in));
    connid                          = calloc(1, sizeof(struct rdma_cm_id));
    void                *rcvbuf     = calloc(1, pagesize);
    void                *sndbuf     = calloc(1, pagesize);
    if (!rcvbuf || !sndbuf || !client_id)
    {
        fprintf(stderr, "Memory allocation failed!\n");
        goto out;
    }

    // Block until client connects
    //printf("Waiting for client...\n");
    ret = rdma_get_request(sockid, &connid);
    if (ret)
    {
        fprintf(stderr, "Failed to read incoming connection request.\n");
        goto out_broken;
    }

    // Accept client
    ret = rdma_accept(connid, NULL);
    if (ret)
    {
        fprintf(stderr, "Could not accept connection.\n");
        goto out_reject;
    }
    
    // Register memory buffers for metadata exchange
    ctx.recv_mr = rdma_reg_msgs(connid, rcvbuf, pagesize);
    if (!ctx.recv_mr)
    {
        fprintf(stderr, "Could not register memory (%lu bytes)\n", pagesize);
        goto out_broken;
    }
    ctx.send_mr = rdma_reg_msgs(connid, sndbuf, pagesize);
    if (!ctx.send_mr)
    {
        fprintf(stderr, "Could not register memory (%lu bytes)\n", pagesize);
        goto out_broken;
    }

    // Main offload function - runs until the client terminates the connection.
    ctx.connid = connid;

    int stat = offload_main(&ctx);
    ret = MPI_Finalize();
    if (ret)
    {
        fprintf(stderr, "MPI could not finalize!!!\n");
        return 1;
    }
    return stat;

out_reject:
    rdma_reject(connid, NULL, 0);
out_broken:
    rdma_destroy_ep(sockid);
    rdma_freeaddrinfo(host_res);
out:
    return 1;
}