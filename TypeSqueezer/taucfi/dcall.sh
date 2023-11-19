#!/bin/bash -v
export DYNINST_ROOT=${HOME}/TypeSqueezer-master/dyninst-9.3.1
export LD_LIBRARY_PATH=$DYNINST_ROOT/install/lib:$LD_LIBRARY_PATH
export DYNINSTAPI_RT_LIB=$DYNINST_ROOT/install/lib/libdyninstAPI_RT.so
export DYNINST_INCLUDE=$DYNINST_ROOT/install/include
export DYNINST_LIB=$DYNINST_ROOT/install/lib
make
