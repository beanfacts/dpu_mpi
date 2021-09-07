#include "get_ip.h"
#include "dpulib.h"

// uint64_t pagesize;

// Initiate connection to DPU so it can be used for collective communication.
DPUContext *DPU_Init(char *dpu_addr, char *dpu_port, int queue_depth)
{   
    int ret = 0;
    DPUContext *ctx = calloc(1, sizeof(DPUContext));
    uint64_t pagesize = sysconf(_SC_PAGESIZE);
    ctx->queue_depth = queue_depth;
    ctx->status = calloc(1, sizeof(DPUStatus) * queue_depth);
    if (!ctx->status)
    {
        fprintf(stderr, "Too many completions - could not allocate.\n");
        return NULL;
    }

    new(struct rdma_addrinfo, hints);
    struct rdma_addrinfo *host_res;
    hints.ai_port_space = RDMA_PS_TCP;

    ret = rdma_getaddrinfo(dpu_addr, dpu_port, &hints, &host_res);
    if (ret)
    {
        fprintf(stderr, "Could not resolve host\n");
        return NULL;
    }

    ctx->connid = malloc(sizeof(struct rdma_cm_id));
    if (!ctx->connid)
    {
        fprintf(stderr, "Could not allocate space for connection context\n");
        return NULL;
    }

    // Create endpoint
    new(struct ibv_qp_init_attr, init_attr);
    init_attr.cap.max_send_wr = 10;
    init_attr.cap.max_recv_wr = 10;
    init_attr.cap.max_send_sge = 10;
    init_attr.cap.max_recv_sge = 10;
    init_attr.sq_sig_all = 1;

    ret = rdma_create_ep(&ctx->connid, host_res, NULL, &init_attr);
    if (ret)
    {
        fprintf(stderr, "Could not create EP.\n");
        return NULL;
    }


    // Connect to DPU
    ret = rdma_connect(ctx->connid, NULL);
    if (ret < 0)
    {
        fprintf(stderr, "Could not connect to DPU: %s\n", strerror(errno));
        return NULL;
    }

    new(struct ibv_qp_init_attr, conn_attr);
    new(struct ibv_qp_attr, conn_qp_attr);
    ret = ibv_query_qp(ctx->connid->qp, &conn_qp_attr, IBV_QP_CAP, &conn_attr);
    if (ret)
    {
        fprintf(stderr, "Could not query QP information.\n");
        rdma_destroy_ep(ctx->connid);
    }

    ctx->qp_max_inline = init_attr.cap.max_inline_data;

    // Allocate send and receive buffers
    void *sndbuf = calloc(1, pagesize);
    void *rcvbuf = calloc(1, pagesize);
    if (!sndbuf || !rcvbuf)
    {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    }

    ctx->meta_send_mr = rdma_reg_msgs(ctx->connid, sndbuf, pagesize);
    if (!ctx->meta_send_mr)
    {
        fprintf(stderr, "SEND: Could not register memory (%lu bytes)\n", pagesize);
        return NULL;
    }

    ctx->meta_recv_mr = rdma_reg_msgs(ctx->connid, rcvbuf, pagesize);
    if (!ctx->meta_recv_mr)
    {
        fprintf(stderr, "RECV: Could not register memory (%lu bytes)\n", pagesize);
        return NULL;
    }

    return ctx;
}


int DPU_Exit(DPUContext *ctx)
{
    int ret;
    struct ibv_wc wc;
    //go away message
    char *msg = "a";
    ret = rdma_post_send(ctx->connid, NULL, msg, 1, NULL, IBV_SEND_INLINE);
    if (ret)
    {
        fprintf(stderr, "We cannot tell the server to go away.\n");
        return 1;
    }
    while (rdma_get_send_comp(ctx->connid, &wc) == 0);
    ret = rdma_disconnect(ctx->connid);
    ret = rdma_dereg_mr(ctx->meta_recv_mr);
    if (ret)
    {
        fprintf(stderr, "Failed to deregister receive MR.\n");
    }
    ret = rdma_dereg_mr(ctx->meta_send_mr);
    if (ret)
    {
        fprintf(stderr, "Failed to deregister send MR.\n");
    }
    free(ctx->status);
    free(ctx);
    return 0;
}



int DPU_MPI_Ialltoall(  DPUContext *ctx,
                        void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        int worldsize )
{
    int ret;
    OffloadReq req = OffloadReq_init_zero;
    struct ibv_mr *send_mr, *recv_mr;
    int send_size, recv_size;

    ret = MPI_Type_size(sendtype, &send_size);
    if (ret)
    {
        fprintf(stderr, "Invalid MPI type\n");
        return -1;
    }

    ret = MPI_Type_size(sendtype, &recv_size);
    if (ret)
    {
        fprintf(stderr, "Invalid MPI type\n");
        return -1;
    }
    
    uint64_t send_alloc = send_size * sendcount * worldsize;
    uint64_t recv_alloc = recv_size * recvcount * worldsize;
    
    send_mr = ibv_reg_mr(ctx->connid->pd, sendbuf, send_alloc, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!send_mr)
    {
        fprintf(stderr, "Error registering send memory region: %s", strerror(errno));
        return -1;
    }

    recv_mr = ibv_reg_mr(ctx->connid->pd, recvbuf, recv_alloc, IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    if (!recv_mr)
    {
        fprintf(stderr, "Error registering recv memory region: %s", strerror(errno));
        return -1;
    }
    
    req.send_buf        = (uint64_t) sendbuf;
    req.send_datatype   = (uint64_t) DPU_MPI_Type_Pack(sendtype);
    req.send_elems      = sendcount;
    req.send_rkey       = send_mr->rkey;
    req.recv_buf        = (uint64_t) recvbuf;
    req.recv_datatype   = (uint64_t) DPU_MPI_Type_Pack(recvtype);
    req.recv_elems      = recvcount;
    req.recv_rkey       = recv_mr->rkey;
    req.cookie          = rand();


    pb_ostream_t out_stream = pb_ostream_from_buffer(ctx->meta_send_mr->addr, ctx->meta_send_mr->length);
    ret = (int) pb_encode(&out_stream, OffloadReq_fields, &req);
    if (!ret)
    {
        fprintf(stderr, "Error serializing into protobuf.\n");
        return -1;
    }

    int flag = IBV_SEND_SIGNALED;

    if (out_stream.bytes_written < ctx->qp_max_inline)
    {
        flag |= IBV_SEND_INLINE;
    }
    
    ret = rdma_post_send(ctx->connid, NULL, ctx->meta_send_mr->addr, out_stream.bytes_written, ctx->meta_send_mr, flag);
    if (ret)
    {
        fprintf(stderr, "Failed to post send to offload engine.\n");
        return -1;
    }

    struct ibv_wc wc;
    while (rdma_get_send_comp(ctx->connid, &wc) == 0);

    for (int i = 0; i < ctx->queue_depth; i++)
    {
        if (ctx->status[i].status == COMP_STATUS_EMPTY)
        {
            ctx->status[i].cookie = req.cookie;
            ctx->status[i].send_mr = send_mr;
            ctx->status[i].recv_mr = recv_mr;
            ctx->status[i].status = COMP_STATUS_PENDING;
            return i;
        }
    }

    return -1;

}

/*
    Poll the completion queue to find completion events.
    Returns:    Completion event position in queue.
                -1 when no event is completed.
                -2 on error.
*/
int DPU_MPI_Poll(DPUContext *ctx)
{
    struct ibv_wc wc;
    int ret;

    // If we are not waiting on a message then do
    if (!ctx->pending_recvs)
    {
        ret = rdma_post_recv(ctx->connid, NULL, ctx->meta_recv_mr->addr, ctx->meta_recv_mr->length, ctx->meta_recv_mr);
        if (ret)
        {
            fprintf(stderr, "Could not post receive!\n");
            return -2;
        }
        ctx->pending_recvs++;
    }
    
    ret = ibv_poll_cq(ctx->connid->recv_cq, 1, &wc);
    if (ret < 0)
    {
        fprintf(stderr, "poll CQ failed %d: %s\n", ret, ibv_wc_status_str(ret));
        return -2;
    }
    else if (ret > 0)
    {

        if (wc.status != IBV_WC_SUCCESS)
        {
            fprintf(stderr, "Transport failure: %s\n", ibv_wc_status_str(wc.status));
            return -2;
        }

        // Subtract number of received messages from pending
        ctx->pending_recvs -= ret;
        
        
        int32_t imm = (int32_t) wc.imm_data;

        // Determine if this event is a completion
        for (int i = 0; i < ctx->queue_depth; i++)
        {
            
            if (ctx->status[i].cookie == imm)
            {
                ctx->status[i].status = COMP_STATUS_OK;
                return i;
            }
            else if (ctx->status[i].cookie == -imm)
            {
                ctx->status[i].status = COMP_STATUS_ERROR;
                return i;
            }
        }
        
        // The DPU sent a completion event but we couldn't match the cookie
        fprintf(stderr, "Invalid completion event from DPU!\n");
        return -2;
    }

    return -1;
}

/*
    Poll the completion queue to find completion events.
    This has an increased latency but lower CPU usage.
    Returns:    Completion event position in queue.
                -1 when no event is completed.
                -2 on error.
*/
int DPU_MPI_Longpoll(DPUContext *ctx)
{
    struct ibv_wc wc;
    int ret;

    // If we are not waiting on a message then do
    if (!ctx->pending_recvs)
    {
        ret = rdma_post_recv(ctx->connid, NULL, ctx->meta_recv_mr->addr, ctx->meta_recv_mr->length, ctx->meta_recv_mr);
        if (ret)
        {
            fprintf(stderr, "Could not post receive!\n");
            return -2;
        }
        ctx->pending_recvs++;
    }
    
    ret = rdma_get_recv_comp(ctx->connid, &wc);
    if (ret < 0)
    {
        fprintf(stderr, "poll CQ failed %d: %s\n", ret, ibv_wc_status_str(ret));
        return -2;
    }
    else if (ret > 0)
    {

        if (wc.status != IBV_WC_SUCCESS)
        {
            fprintf(stderr, "Transport failure: %s\n", ibv_wc_status_str(wc.status));
            return -2;
        }

        // Subtract number of received messages from pending
        ctx->pending_recvs -= ret;
        
        
        int32_t imm = (int32_t) wc.imm_data;

        // Determine if this event is a completion
        for (int i = 0; i < ctx->queue_depth; i++)
        {
            
            if (ctx->status[i].cookie == imm)
            {
                ctx->status[i].status = COMP_STATUS_OK;
                return i;
            }
            else if (ctx->status[i].cookie == -imm)
            {
                ctx->status[i].status = COMP_STATUS_ERROR;
                return i;
            }
        }
        
        // The DPU sent a completion event but we couldn't match the cookie
        fprintf(stderr, "Invalid completion event from DPU!\n");
        return -2;
    }

    return -1;
}

/*
    Test whether an offloaded operation is completed.
    Returns the index if completed. -1 if not. -2 if error.
    If alltoall was called correctly, the cookie should never be negative.
*/
int DPU_MPI_Test(DPUContext *ctx, int cookie)
{
    // Ask network hardware for new recv or check existing ones
    int ret = DPU_MPI_Poll(ctx);
    if (ret == -2)
    {
        fprintf(stderr, "Error performing DPU_MPI_Poll\n");
        return -2;
    }
    
    // Check the completion list for new cookies
    for (int i = 0; i < ctx->queue_depth; i++)
    {
        if (ctx->status[i].cookie == cookie && ctx->status[i].status == COMP_STATUS_OK)
        {

            ret = ibv_dereg_mr(ctx->status[i].send_mr);
            if (ret)
            {
                fprintf(stderr, "failed to cleanup...\n");
            }
            ret = ibv_dereg_mr(ctx->status[i].recv_mr);
            if (ret)
            {
                fprintf(stderr, "failed to cleanup...\n");
            }

            ctx->status[i].status = COMP_STATUS_EMPTY;
            return i;
        }
    }
    return -1;
}

/*
    Wait for a specific offloaded operation to complete.
    Note request can block forever if the cookie is invalid.
*/
int DPU_MPI_Wait(DPUContext *ctx, int cookie)
{
    int x;
    do
    {
        x = DPU_MPI_Test(ctx, cookie);
    }
    while (x == -1);
    
    if (x == -2)
    {
        fprintf(stderr, "Error!!\n");
        return 1;
    }
    
    return 0;
}

int get_cookie(DPUContext *ctx, int index){
   return ctx->status[index].cookie;
}