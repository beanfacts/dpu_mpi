#include "dpucommon.h"

int DPU_MPI_Type_Pack(MPI_Datatype input_type)
{ 
    if      (input_type == MPI_CHAR)                        { return DPU_MPI_CHAR; }
    else if (input_type == MPI_UNSIGNED_CHAR)               { return DPU_MPI_UNSIGNED_CHAR; }
    else if (input_type == MPI_SHORT)                       { return DPU_MPI_SHORT; }
    else if (input_type == MPI_UNSIGNED_SHORT)              { return DPU_MPI_UNSIGNED_SHORT; }
    else if (input_type == MPI_INT)                         { return DPU_MPI_INT; }
    else if (input_type == MPI_UNSIGNED)                    { return DPU_MPI_UNSIGNED; }
    else if (input_type == MPI_LONG)                        { return DPU_MPI_LONG; }
    else if (input_type == MPI_UNSIGNED_LONG)               { return DPU_MPI_UNSIGNED_LONG; }
    else if (input_type == MPI_LONG_LONG_INT)               { return DPU_MPI_LONG_LONG_INT; }
    else if (input_type == MPI_FLOAT)                       { return DPU_MPI_FLOAT; }
    else if (input_type == MPI_DOUBLE)                      { return DPU_MPI_DOUBLE; }
    else if (input_type == MPI_LONG_DOUBLE)                 { return DPU_MPI_LONG_DOUBLE; }
    else if (input_type == MPI_BYTE)                        { return DPU_MPI_BYTE; }
    else if (input_type == MPI_UINT32_T)                    { return DPU_MPI_UINT32_T; }
    else if (input_type == MPI_DATATYPE_NULL)               { return DPU_MPI_DATATYPE_NULL; }
    else if (input_type == MPI_PACKED)                      { return DPU_MPI_PACKED; }
    else if (input_type == MPI_INT8_T)                      { return DPU_MPI_INT8_T ; }
    else if (input_type == MPI_SIGNED_CHAR)                 { return DPU_MPI_SIGNED_CHAR; }
    else if (input_type == MPI_FLOAT_INT)                   { return DPU_MPI_FLOAT_INT; }
    else if (input_type == MPI_DOUBLE_INT)                  { return DPU_MPI_DOUBLE_INT; }
    else if (input_type == MPI_LONG_DOUBLE_INT)             { return DPU_MPI_LONG_DOUBLE_INT; }
    else if (input_type == MPI_UINT8_T)                     { return DPU_MPI_UINT8_T; }
    else if (input_type == MPI_LONG_INT)                    { return DPU_MPI_LONG_INT; }
    else if (input_type == MPI_SHORT_INT)                   { return DPU_MPI_SHORT_INT; }
    else if (input_type == MPI_INT16_T)                     { return DPU_MPI_INT16_T ; }
    else if (input_type == MPI_2INT)                        { return DPU_MPI_2INT; }
    else if (input_type == MPI_UINT16_T)                    { return DPU_MPI_UINT16_T; }
    else if (input_type == MPI_INT32_T)                     { return DPU_MPI_INT32_T; }
    else if (input_type == MPI_INT64_T)                     { return DPU_MPI_INT64_T; }
    else if (input_type == MPI_UINT64_T)                    { return DPU_MPI_UINT64_T; }
    else if (input_type == MPI_AINT)                        { return DPU_MPI_AINT; }
    else if (input_type == MPI_OFFSET)                      { return DPU_MPI_OFFSET; }
    else if (input_type == MPI_C_BOOL)                      { return DPU_MPI_C_BOOL; }
    else if (input_type == MPI_C_COMPLEX)                   { return DPU_MPI_C_COMPLEX; }
    else if (input_type == MPI_C_FLOAT_COMPLEX)             { return DPU_MPI_C_FLOAT_COMPLEX; }
    else if (input_type == MPI_C_DOUBLE_COMPLEX)            { return DPU_MPI_C_DOUBLE_COMPLEX; }
    else if (input_type == MPI_C_LONG_DOUBLE_COMPLEX)       { return DPU_MPI_C_LONG_DOUBLE_COMPLEX; }
    else if (input_type == MPI_CXX_BOOL)                    { return DPU_MPI_CXX_BOOL; }
    else if (input_type == MPI_CXX_COMPLEX)                 { return DPU_MPI_CXX_COMPLEX; }
    else if (input_type == MPI_CXX_DOUBLE_COMPLEX)          { return DPU_MPI_CXX_DOUBLE_COMPLEX; }
    else if (input_type == MPI_CXX_FLOAT_COMPLEX)           { return DPU_MPI_CXX_FLOAT_COMPLEX; }
    else if (input_type == MPI_CXX_LONG_DOUBLE_COMPLEX)     { return DPU_MPI_CXX_LONG_DOUBLE_COMPLEX; }
    return -1;
}


MPI_Datatype DPU_MPI_Type_Unpack(int input_type)
{
    switch (input_type) {
        case DPU_MPI_CHAR:
            return MPI_CHAR;        
        case DPU_MPI_UNSIGNED_CHAR:
            return MPI_UNSIGNED_CHAR;
        case DPU_MPI_SIGNED_CHAR:
            return MPI_SIGNED_CHAR;
        case DPU_MPI_SHORT:
            return MPI_SHORT;
        case DPU_MPI_UNSIGNED_SHORT:
            return MPI_UNSIGNED_SHORT;
        case DPU_MPI_INT:
            return MPI_INT;
        case DPU_MPI_UNSIGNED:
            return MPI_UNSIGNED;
        case DPU_MPI_LONG:
            return MPI_LONG;
        case DPU_MPI_UNSIGNED_LONG:
            return MPI_UNSIGNED_LONG;
        case DPU_MPI_LONG_LONG_INT:
            return MPI_LONG_LONG_INT;
        case DPU_MPI_FLOAT:
            return MPI_FLOAT;
        case DPU_MPI_DOUBLE:
            return MPI_DOUBLE;
        case DPU_MPI_LONG_DOUBLE:
            return MPI_LONG_DOUBLE;
        case DPU_MPI_BYTE:
            return MPI_BYTE;
        case DPU_MPI_UINT32_T:
            return MPI_UINT32_T;
        case DPU_MPI_DATATYPE_NULL:
            return MPI_DATATYPE_NULL;
        case DPU_MPI_PACKED:
            return MPI_PACKED;
        case DPU_MPI_FLOAT_INT:
            return MPI_FLOAT_INT;    
        case DPU_MPI_DOUBLE_INT:
            return MPI_DOUBLE_INT;      
        case DPU_MPI_LONG_DOUBLE_INT:
            return MPI_LONG_DOUBLE_INT; 
        case DPU_MPI_LONG_INT:
            return MPI_LONG_INT;       
        case DPU_MPI_SHORT_INT:
            return MPI_SHORT_INT;     
        case DPU_MPI_2INT:
            return MPI_2INT;
        case DPU_MPI_INT8_T:
            return MPI_INT8_T; 
        case DPU_MPI_UINT8_T:
            return MPI_UINT8_T;  
        case DPU_MPI_INT16_T:
            return MPI_INT16_T; 
        case DPU_MPI_UINT16_T:
            return MPI_UINT16_T; 
        case DPU_MPI_INT32_T:
            return MPI_INT32_T;
        case DPU_MPI_INT64_T:
            return MPI_INT64_T;
        case DPU_MPI_UINT64_T:
            return MPI_UINT64_T; 
        case DPU_MPI_AINT:
            return MPI_AINT;
        case DPU_MPI_OFFSET:
            return MPI_OFFSET; 
        case DPU_MPI_C_BOOL:
            return MPI_C_BOOL;
        case DPU_MPI_C_COMPLEX:
            return MPI_C_COMPLEX;
        case DPU_MPI_C_FLOAT_COMPLEX:
            return MPI_C_FLOAT_COMPLEX;
        case DPU_MPI_C_DOUBLE_COMPLEX:
            return MPI_C_DOUBLE_COMPLEX;
        case DPU_MPI_C_LONG_DOUBLE_COMPLEX:
            return MPI_C_LONG_DOUBLE_COMPLEX;
        case DPU_MPI_CXX_BOOL:
            return MPI_CXX_BOOL;
        case DPU_MPI_CXX_COMPLEX:
            return MPI_CXX_COMPLEX;
        case DPU_MPI_CXX_FLOAT_COMPLEX:
            return MPI_CXX_FLOAT_COMPLEX;
        case DPU_MPI_CXX_DOUBLE_COMPLEX:
            return MPI_CXX_DOUBLE_COMPLEX;
        case DPU_MPI_CXX_LONG_DOUBLE_COMPLEX:
            return MPI_CXX_LONG_DOUBLE_COMPLEX;
    }
    return 0;
}

int init_mpi(int *argc, char ***argv, int *rank, int *size)
{
    int ret = MPI_Init(argc, argv);
    MPI_Comm_rank(MPI_COMM_WORLD, rank);
    MPI_Comm_size(MPI_COMM_WORLD, size);
    return ret;
}


int test_mpi(int rank)
{
    int test = 12345;
    if (rank != 0)
    {
        test = 0;
    }
    
    MPI_Bcast(&test, 1, MPI_UINT32_T, 0, MPI_COMM_WORLD);
    if (test != 12345)
    {
        return 1;
    }
    return 0;
}

int rdma_post_sendv_im(struct rdma_cm_id *id, void *context, struct ibv_sge *sgl,
		int nsge, int flags, uint32_t data )
{
	struct ibv_send_wr wr, *bad;

	wr.wr_id      = (uintptr_t) context;
	wr.next       = NULL;
	wr.sg_list    = sgl;
	wr.num_sge    = nsge;
	wr.opcode     = IBV_WR_SEND_WITH_IMM;
	wr.send_flags = flags;
    wr.imm_data   = data;

	return rdma_seterrno(ibv_post_send(id->qp, &wr, &bad));
}


int rdma_post_send_im(struct rdma_cm_id *id, void *context, void *addr,
	       size_t length, struct ibv_mr *mr, int flags, uint32_t data)
{
	struct ibv_sge sge;

	sge.addr = (uint64_t) (uintptr_t) addr;
	sge.length = (uint32_t) length;
	sge.lkey = mr ? mr->lkey : 0;

	return rdma_post_sendv_im(id, context, &sge, 1, flags, data);
}

int rdma_ext_post_send(struct rdma_cm_id *id, void *context, void *addr, size_t length, struct ibv_mr *mr, int flags)
{
    struct ibv_sge sge;
    struct ibv_send_wr wr, *bad;
    sge.addr = (uint64_t) (uintptr_t) addr;
	sge.length = (uint32_t) length;
	sge.lkey = mr ? mr->lkey : 0;
    
    wr.wr_id = (uintptr_t) context;
    wr.next = NULL;
    wr.sg_list = &sge;
    wr.num_sge = 1;
    wr.imm_data = 1999;
    wr.opcode = IBV_WR_SEND_WITH_IMM;

    return rdma_seterrno(ibv_post_send(id->qp, &wr, &bad));
}

int rdma_ext_post_write_imm(struct rdma_cm_id *id, void *context, void *addr,
	    size_t length, struct ibv_mr *mr, int flags,
		uint64_t remote_addr, uint32_t rkey, uint32_t imm_data)
{
    struct ibv_sge sge;
    struct ibv_send_wr wr, *bad;
    sge.addr = (uint64_t) (uintptr_t) addr;
	sge.length = (uint32_t) length;
	sge.lkey = mr ? mr->lkey : 0;
	wr.wr_id = (uintptr_t) context;
	wr.next = NULL;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_RDMA_WRITE_WITH_IMM;
    wr.imm_data = imm_data;
	wr.send_flags = flags;
	wr.wr.rdma.remote_addr = remote_addr;
	wr.wr.rdma.rkey = rkey;

    return rdma_seterrno(ibv_post_send(id->qp, &wr, &bad));
}