NANOPB_DIR = protobuf

# Compiler flags to enable all warnings & debug info
CFLAGS = -Wall -O1
CFLAGS += "-I$(NANOPB_DIR)"

# C source code files that are required
CSRC += $(NANOPB_DIR)/common.pb.c
CSRC += $(NANOPB_DIR)/pb_encode.c  # The nanopb encoder
CSRC += $(NANOPB_DIR)/pb_decode.c  # The nanopb decoder
CSRC += $(NANOPB_DIR)/pb_common.c  # The nanopb common parts

LIBS = -lrdmacm -libverbs

HOSTS = 8
# HOST_ALLOC = thor001,thor002,thor003,thor004,thor005,thor006
# BF_ALLOC = thor-bf01,thor-bf02,thor-bf03,thor-bf04,thor-bf05,thor-bf06,
HOST_ALLOC = thor001,thor002,thor003,thor004,thor012,thor013,thor014,thor015
BF_ALLOC = thor-bf01,thor-bf02,thor-bf03,thor-bf04,thor-bf12,thor-bf13,thor-bf14,thor-bf15
BF86S = 2
BF86_ALLOC = thor003,thor004

#optional for using another x86 system as the offload 
runbf86:
	mpirun -np $(BF86S) --mca btl self,tcp --map-by node -H $(BF86_ALLOC) ./bf86.o

bf86:
	mpicc $(CFLAGS) get_ip.c common.c main_bf.c $(CSRC) $(LIBS) -o bf86.o

# this use the same host binary as runhost_lib but with differnet parameter for ip addresses
runhost_lib86:
	mpirun -report-bindings -np $(HOSTS) --mca btl self,tcp --map-by node -H $(HOST_ALLOC) ./host_lib.o 2


#main make functions 
host_lib:
	mpicc $(CFLAGS) get_ip.c dpucommon.c dpulib.c main_host_use_lib.c $(CSRC) $(LIBS) -o host_lib.o 
bf:
	mpicc $(CFLAGS) get_ip.c dpucommon.c main_bf.c $(CSRC) $(LIBS) -o bf.o

runbf:
	mpirun -np $(HOSTS) --mca btl self,tcp --map-by node -H $(BF_ALLOC) ./bf.o
runhost_lib:
	mpirun -report-bindings -np $(HOSTS) --mca btl self,tcp --map-by node -H $(HOST_ALLOC) ./host_lib.o 100

