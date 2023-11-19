#!/bin/bash -v
export DYNINST_ROOT=${HOME}/TypeSqueezer-master/dyninst-9.3.1
export LD_LIBRARY_PATH=$DYNINST_ROOT/install/lib:$LD_LIBRARY_PATH
export DYNINSTAPI_RT_LIB=$DYNINST_ROOT/install/lib/libdyninstAPI_RT.so
export DYNINST_INCLUDE=$DYNINST_ROOT/install/include
export DYNINST_LIB=$DYNINST_ROOT/install/lib
cd dump
objdump -d "../bin/$1" > "$1.dump"
objdump -h "../bin/$1" > "../misc/tmp/$1_h.tmp"
cd ../at
python3 findat.py $1 | wc -l
python3 findat.py $1 > "$1.at"
cd ../misc/dump2
./dumpAnalyze $1
cd ../../directcall/Dcall_tauCFI
./eg $1
cd ../Dcall_TypeArmor
./eg $1
cd ../../taucfi
./eg $1
cd ..
. ./envsetup.sh
cp "../../bincfi/bin/$1" ./server-bins
cd server-bins
../run-ta-static.sh $1
cd ../pinbench
mkdir -p $1
cp "../bin/$1" "$1"

