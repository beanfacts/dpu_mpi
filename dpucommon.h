#ifndef BF_COMMON_H
#define BF_COMMON_H

// Async poll
#include <poll.h>

// Required libraries
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// RDMA_CM libraries
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>

// MPI duh
#include <mpi.h>
#include <pmix.h>

// Library to serialize request data
#include "protobuf/pb_decode.h"
#include "protobuf/pb_encode.h"
#include "protobuf/common.pb.h"

/*  
    Message type identifiers
    MID_EMPTY:      Empty message (pingpong)
    MID_METADATA:   Message contains metadata but no actual data
    MID_WITH_DATA:  Message contains both required metadata and send data
*/
typedef enum {
    MID_EMPTY       = 0,
    MID_METADATA    = 1,
    MID_WITH_DATA   = 2
} MessageID;

/*
    For custom fixed-length datatypes, you can add an entry in here
    and append the pack/unpack functions with your custom datatype
    as long as your datatype can be processed by MPI_Type_size()
*/
typedef enum {
    DPU_MPI_CHAR                    = 1,
    DPU_MPI_UNSIGNED_CHAR           = 2,
    DPU_MPI_SHORT                   = 3,
    DPU_MPI_UNSIGNED_SHORT          = 4,
    DPU_MPI_INT                     = 5,
    DPU_MPI_UNSIGNED                = 6,
    DPU_MPI_LONG                    = 7,
    DPU_MPI_UNSIGNED_LONG           = 8,
    DPU_MPI_LONG_LONG_INT           = 9,
    DPU_MPI_FLOAT                   = 10,
    DPU_MPI_DOUBLE                  = 11,
    DPU_MPI_LONG_DOUBLE             = 12,
    DPU_MPI_BYTE                    = 13,
    DPU_MPI_UINT32_T                = 14,
    DPU_MPI_DATATYPE_NULL           = 15,
    DPU_MPI_PACKED                  = 16,
    DPU_MPI_INT8_T                  = 17,
    DPU_MPI_SIGNED_CHAR             = 18,
    DPU_MPI_FLOAT_INT               = 19,
    DPU_MPI_DOUBLE_INT              = 20,
    DPU_MPI_LONG_DOUBLE_INT         = 21,
    DPU_MPI_UINT8_T                 = 22,
    DPU_MPI_LONG_INT                = 23,
    DPU_MPI_SHORT_INT               = 24,
    DPU_MPI_INT16_T                 = 25,
    DPU_MPI_2INT                    = 26,
    DPU_MPI_UINT16_T                = 27,
    DPU_MPI_INT32_T                 = 28,
    DPU_MPI_INT64_T                 = 29,
    DPU_MPI_UINT64_T                = 30,
    DPU_MPI_AINT                    = 31,
    DPU_MPI_OFFSET                  = 32,
    DPU_MPI_C_BOOL                  = 33,
    DPU_MPI_C_COMPLEX               = 34,
    DPU_MPI_C_FLOAT_COMPLEX         = 35,
    DPU_MPI_C_DOUBLE_COMPLEX        = 36,
    DPU_MPI_C_LONG_DOUBLE_COMPLEX   = 37,
    DPU_MPI_CXX_BOOL                = 38,
    DPU_MPI_CXX_COMPLEX             = 39,
    DPU_MPI_CXX_DOUBLE_COMPLEX      = 40,
    DPU_MPI_CXX_FLOAT_COMPLEX       = 41,
    DPU_MPI_CXX_LONG_DOUBLE_COMPLEX = 42,
} DPU_MPI_Datatype;

/*
    Abomination of a function that serializes MPI datatypes for network 
    transmission. Returns an integer corresponding to the item in the
    DPU_MPI_Datatype enum.
*/
int DPU_MPI_Type_Pack(MPI_Datatype input_type);

/*
    Converts the DPU MPI Datatype into the actual datatype to be used
    by MPI functions.
*/
MPI_Datatype DPU_MPI_Type_Unpack(int input_type);

#define COMP_STATUS_EMPTY       0
#define COMP_STATUS_OK          1
#define COMP_STATUS_PENDING     2
#define COMP_STATUS_ERROR       3

#define IMMD_TYPE_METADATA      0
#define IMMD_TYPE_DIRECT        1
#define IMMD_TYPE_INLINE        2

/* Initialize MPI */
int init_mpi(int *argc, char ***argv, int *rank, int *size);

// Check if MPI is working.
int test_mpi(int rank);

/* 
    modified version of rdma post send that send immediate
    and includes inline data
*/
int rdma_post_sendv_im(struct rdma_cm_id *id, void *context, struct ibv_sge *sgl,
		int nsge, int flags, uint32_t data );


int rdma_post_send_im(struct rdma_cm_id *id, void *context, void *addr,
	       size_t length, struct ibv_mr *mr, int flags, uint32_t data);

/* Allocate new item on stack */
#define new(type, x) type x; memset(&x, 0, sizeof(x)) 

int  rdma_ext_post_send(   struct rdma_cm_id *id, void *context, void *addr, 
                        size_t length, struct ibv_mr *mr, int flags);


int rdma_ext_post_write_imm(struct rdma_cm_id *id, void *context, void *addr,
	    size_t length, struct ibv_mr *mr, int flags,
		uint64_t remote_addr, uint32_t rkey, uint32_t imm_data);

#endif
