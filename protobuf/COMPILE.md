# Compile DPU offload Protobuf

You will need protobuf-compiler.


In this directory, perform:
```sh
protoc common.proto -o common.pb
./generator/nanopb_generator.py common.pb
```