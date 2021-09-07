#ifndef HOST_LIB
#define HOST_LIB

#include "dpucommon.h"

/* Stores data to track DPU completion status */
typedef struct {
    int                 cookie;
    int                 status;
    struct ibv_mr       *send_mr;
    struct ibv_mr       *recv_mr;
} DPUStatus;

/* Stores data needed to interface w/ DPU */
typedef struct {
    struct rdma_cm_id   *connid;
    struct ibv_mr       *meta_send_mr;
    struct ibv_mr       *meta_recv_mr;
    int                 pending_sends;
    int                 pending_recvs;
    int                 qp_max_inline;
    DPUStatus           *status;
    int                 queue_depth;
} DPUContext;

DPUContext *DPU_Init(char *dpu_addr, char *dpu_port, int queue_depth); // dpustatus completions -> int queue_depth

int DPU_Exit(DPUContext *ctx);
int DPU_MPI_Ialltoall(  DPUContext *ctx,
                        void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        int worldsize);
int DPU_MPI_Poll(DPUContext *ctx);
int DPU_MPI_Test(DPUContext *ctx, int cookie);
int DPU_MPI_Wait(DPUContext *ctx, int cookie);
int get_cookie(DPUContext *ctx,int index);

#endif