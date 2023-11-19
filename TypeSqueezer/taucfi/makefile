# DYNINST_INCLUDE and DYNINST_LIB should be set to locations
# where Dyninst header and library files were installed, respectively

CC=gcc
CXX=g++

DYNINST_ROOT=${HOME}/TypeSqueezer-master/dyninst-9.3.1

DYNINST_INCLUDE  =    -I${DYNINST_ROOT}/install/include

eg:eg.o
	$(CXX) eg.o -std=c++0x -L${DYNINST_ROOT}/install/lib -ldwarf ${DYNINST_ROOT}/install/lib/libstackwalk.so.9.3.1 ${DYNINST_ROOT}/install/lib/libpcontrol.so.9.3.1 ${DYNINST_ROOT}/install/lib/libdynDwarf.so.9.3.1 ${DYNINST_ROOT}/install/lib/libdynElf.so.9.3.1 ${DYNINST_ROOT}/install/lib/libdyninstAPI.so.9.3.1 ${DYNINST_ROOT}/install/lib/libinstructionAPI.so.9.3.1 ${DYNINST_ROOT}/install/lib/libparseAPI.so.9.3.1 ${DYNINST_ROOT}/install/lib/libcommon.so.9.3.1 ${DYNINST_ROOT}/install/lib/libsymtabAPI.so.9.3.1 ${DYNINST_ROOT}/install/lib/libpatchAPI.so.9.3.1 -lboost_thread -lboost_system -o eg
eg.o:eg.cpp
	$(CXX) -c $(CFLAGS) $(CFLAGS) $(DYNINST_INCLUDE) -std=c++0x eg.cpp -o eg.o/
