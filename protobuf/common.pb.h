/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.6-dev */

#ifndef PB_COMMON_PB_H_INCLUDED
#define PB_COMMON_PB_H_INCLUDED
#include "pb.h"

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _OffloadCompletion { 
    uint32_t status; 
    uint32_t cookie; 
} OffloadCompletion;

typedef struct _OffloadReq { 
    uint64_t send_buf; 
    uint32_t send_elems; 
    uint64_t send_datatype; 
    uint32_t send_rkey; 
    uint64_t recv_buf; 
    uint32_t recv_elems; 
    uint64_t recv_datatype; 
    uint32_t recv_rkey; 
    uint32_t cookie; 
} OffloadReq;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define OffloadReq_init_default                  {0, 0, 0, 0, 0, 0, 0, 0, 0}
#define OffloadCompletion_init_default           {0, 0}
#define OffloadReq_init_zero                     {0, 0, 0, 0, 0, 0, 0, 0, 0}
#define OffloadCompletion_init_zero              {0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define OffloadCompletion_status_tag             1
#define OffloadCompletion_cookie_tag             2
#define OffloadReq_send_buf_tag                  1
#define OffloadReq_send_elems_tag                2
#define OffloadReq_send_datatype_tag             3
#define OffloadReq_send_rkey_tag                 4
#define OffloadReq_recv_buf_tag                  5
#define OffloadReq_recv_elems_tag                6
#define OffloadReq_recv_datatype_tag             7
#define OffloadReq_recv_rkey_tag                 8
#define OffloadReq_cookie_tag                    9

/* Struct field encoding specification for nanopb */
#define OffloadReq_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT64,   send_buf,          1) \
X(a, STATIC,   REQUIRED, UINT32,   send_elems,        2) \
X(a, STATIC,   REQUIRED, UINT64,   send_datatype,     3) \
X(a, STATIC,   REQUIRED, UINT32,   send_rkey,         4) \
X(a, STATIC,   REQUIRED, UINT64,   recv_buf,          5) \
X(a, STATIC,   REQUIRED, UINT32,   recv_elems,        6) \
X(a, STATIC,   REQUIRED, UINT64,   recv_datatype,     7) \
X(a, STATIC,   REQUIRED, UINT32,   recv_rkey,         8) \
X(a, STATIC,   REQUIRED, UINT32,   cookie,            9)
#define OffloadReq_CALLBACK NULL
#define OffloadReq_DEFAULT NULL

#define OffloadCompletion_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, UINT32,   status,            1) \
X(a, STATIC,   REQUIRED, UINT32,   cookie,            2)
#define OffloadCompletion_CALLBACK NULL
#define OffloadCompletion_DEFAULT NULL

extern const pb_msgdesc_t OffloadReq_msg;
extern const pb_msgdesc_t OffloadCompletion_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define OffloadReq_fields &OffloadReq_msg
#define OffloadCompletion_fields &OffloadCompletion_msg

/* Maximum encoded size of messages (where known) */
#define OffloadCompletion_size                   12
#define OffloadReq_size                          74

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
