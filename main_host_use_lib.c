#include "dpulib.h"
#include "get_ip.h"


/* MPI stuff */
int rank, worldsize;



int main(int argc, char **argv)
{   
    
    int ret;
    uint64_t pagesize = sysconf(_SC_PAGESIZE);

    // Initialize MPI
    init_mpi(&argc, &argv, &rank, &worldsize);
    printf("(%d) MPI Size: %d\n", rank, worldsize);
    
    if (argc != 2)
    {
        fprintf(stderr, "Invalid usage...\n");
        return 1;
    }
    
    ret = test_mpi(rank);
    if (ret)
    {
        fprintf(stderr, "MPI Basic Check failed\n");
        return 1;
    }
    
    srand48(time(0));
    
    char *ip    = offset_addr("ib0_mlx5", atoi(argv[1]));
    char *port  = "9999";
    
    // port is fixed
    DPUContext *ctx = DPU_Init(ip, port, 32);
    if (!ctx)
    {
        return 1;
    }

    printf("DPU active\n");
    
    void *sndbuf, *rcvbuf;
    ret = posix_memalign(&sndbuf, pagesize, pagesize);
    if (ret)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    ret = posix_memalign(&rcvbuf, pagesize, pagesize);
    if (ret)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    for (int r = 0; r < 100; r++)
    {
        // Fill send buffer with unique data
        for (int i = 0; i < worldsize; i++)
        {
            ((int *) sndbuf)[i] = rank * worldsize + i;
        }

        for (int i = 0; i < worldsize; i++)
        {
            ((int *) rcvbuf)[i] = 0;
        }
        
        printf("(%d) Performing alltoall...\n", rank);
        int index = DPU_MPI_Ialltoall(ctx, sndbuf, 1, MPI_UINT32_T, rcvbuf, 1, MPI_UINT32_T, worldsize);
        if (index < 0)
        {
            fprintf(stderr, "Bad cookie.\n");
            return 1;
        }

        int cookie1 = get_cookie(ctx, index);
        
        printf("(%d) Polling1...\n", rank);
        ret = DPU_MPI_Wait(ctx, cookie1);
        if (ret)
        {
            fprintf(stderr, "DPU Poll Failed\n");
            return 1;
        }

        printf("(%d) Data in receive Buffer: ", rank);
        for (int i = 0; i < worldsize; i++)
        {
            printf("%d ",((int *) rcvbuf)[i]);
        }
        printf("\n");
    }

    printf("Exiting...\n");
    DPU_Exit(ctx);
    printf("Finalizing MPI...\n");
    MPI_Finalize();
    printf("Goodbye\n");
    return 0;
}