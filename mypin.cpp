/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2017 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#include "pin.H"
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <set>
#include <vector>
#include <string>

#if defined(TARGET_MAC)
#define FUNC_PREFIX "_"
#else
#define FUNC_PREFIX
#endif

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
using namespace std;
std::ofstream TraceFileCfg;
std::ofstream TraceFileType;
std::ofstream TraceFileRetType;

PIN_LOCK pinLock;

set<string> icNextSet;
string filename;
map<string , int> icallAddrIndexMap;
map<int , string> icallLineIndexMap;
map<string , string> atLineIndexMap;
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "", "specify file name for output");
map<string , int> countMap;


set<long> myset;
map<ADDRINT , vector<ADDRINT> > mymap;

/* ===================================================================== */
VOID PrintIcallArgs(ADDRINT rip,ADDRINT trg,ADDRINT arg1,ADDRINT arg2,ADDRINT arg3,ADDRINT arg4,ADDRINT arg5,ADDRINT arg6 , THREADID tid)
{

	long sig = rip * trg;
    PIN_GetLock(&pinLock , PIN_GetPid() + 1);
	if(myset.count(sig) == 0){
		myset.insert(sig);
		TraceFileCfg<<hex<<"icalladdress:\t"<<rip<<"\ttargetaddress:\t"<<trg<<endl;
	}
    PIN_ReleaseLock(&pinLock);





    static std::map<ADDRINT,uint32_t> visited_icall_AT_pairs;
    if(visited_icall_AT_pairs.count(rip * trg) == 0)
    {
        visited_icall_AT_pairs[rip * trg] = (uint32_t)1;
    }
    else
    {
        if(visited_icall_AT_pairs[rip * trg] >= (uint32_t)100)
        {
            return;
        }
        else
        {
            visited_icall_AT_pairs[rip * trg] += (uint32_t)1;
        }
    }

    uint64_t mem_val1,mem_val2,mem_val3,mem_val4,mem_val5,mem_val6;
    int ret_size1 = PIN_SafeCopy(&mem_val1,(const void *)arg1,8);
    int ret_size2 = PIN_SafeCopy(&mem_val2,(const void *)arg2,8);
    int ret_size3 = PIN_SafeCopy(&mem_val3,(const void *)arg3,8);
    int ret_size4 = PIN_SafeCopy(&mem_val4,(const void *)arg4,8);
    int ret_size5 = PIN_SafeCopy(&mem_val5,(const void *)arg5,8);
    int ret_size6 = PIN_SafeCopy(&mem_val6,(const void *)arg6,8);

    string t1,t2,t3,t4,t5,t6;
    t1 = t2 = t3 = t4 = t5 = t6 = "u";
    if(arg1 != 0)
    {
        if(ret_size1 > 0)
        {
            uint64_t tmp;
            if(mem_val1 == 0)
            {
                t1  = "p.u";
            }
            else if(PIN_SafeCopy(&tmp,(const void *)mem_val1,8) > 0)
            {
                t1 = "p.p";
            }
            else
            {
                t1 = "p.i";
            }
        }
        else if(ret_size1 == 0)
        {
            t1 = "i";
        }
        else
        {
            cerr<<"wtf?"<<endl;
        }
    }
    if(arg2 != 0)
    {
        if(ret_size2 > 0)
        {
            uint64_t tmp;
            if(mem_val2 == 0)
            {
                t2  = "p.u";
            }
            else if(PIN_SafeCopy(&tmp,(const void *)mem_val2,8) > 0)
            {
                t2 = "p.p";
            }
            else
            {
                t2 = "p.i";
            }
        }
        else if(ret_size2 == 0)
        {
            t2 = "i";
        }
        else
        {
            cerr<<"wtf?"<<endl;
        }
    }
    if(arg3 != 0)
    {
        if(ret_size3 > 0)
        {
            uint64_t tmp;
            if(mem_val3 == 0)
            {
                t3  = "p.u";
            }
            else if(PIN_SafeCopy(&tmp,(const void *)mem_val3,8) > 0)
            {
                t3 = "p.p";
            }
            else
            {
                t3 = "p.i";
            }
        }
        else if(ret_size3 == 0)
        {
            t3 = "i";
        }
        else
        {
            cerr<<"wtf?"<<endl;
        }
    }
    if(arg4 != 0)
    {
        if(ret_size4 > 0)
        {
            uint64_t tmp;
            if(mem_val4 == 0)
            {
                t4  = "p.u";
            }
            else if(PIN_SafeCopy(&tmp,(const void *)mem_val4,8) > 0)
            {
                t4 = "p.p";
            }
            else
            {
                t4 = "p.i";
            }
        }
        else if(ret_size4 == 0)
        {
            t4 = "i";
        }
        else
        {
            cerr<<"wtf?"<<endl;
        }
    }
    if(arg5 != 0)
    {
        if(ret_size5 > 0)
        {
            uint64_t tmp;
            if(mem_val5 == 0)
            {
                t5  = "p.u";
            }
            else if(PIN_SafeCopy(&tmp,(const void *)mem_val5,8) > 0)
            {
                t5 = "p.p";
            }
            else
            {
                t5 = "p.i";
            }
        }
        else if(ret_size5 == 0)
        {
            t5 = "i";
        }
        else
        {
            cerr<<"wtf?"<<endl;
        }
    }
    if(arg6 != 0)
    {
        if(ret_size6 > 0)
        {
            uint64_t tmp;
            if(mem_val6 == 0)
            {
                t6  = "p.u";
            }
            else if(PIN_SafeCopy(&tmp,(const void *)mem_val6,8) > 0)
            {
                t6 = "p.p";
            }
            else
            {
                t6 = "p.i";
            }
        }
        else if(ret_size6 == 0)
        {
            t6 = "i";
        }
        else
        {
            cerr<<"wtf?"<<endl;
        }
    }
    PIN_GetLock(&pinLock , tid + 1);
    TraceFileType<<std::hex<<rip<<'\t'<<trg<<'\t'<<t1<<'\t'<<t2<<'\t'<<t3<<'\t'<<t4<<'\t'<<t5<<'\t'<<t6<<'\n';
    PIN_ReleaseLock(&pinLock);

}

/* ===================================================================== */

VOID PrintRetArg(ADDRINT rip,ADDRINT trg,ADDRINT arg1 , THREADID tid){

    ostringstream insAddrStream;
    insAddrStream<<std::hex<<rip;
    ostringstream targetAddrStream;
    targetAddrStream<<std::hex<<trg;

    string instAddr = insAddrStream.str();
    string targetAddr = targetAddrStream.str();

    if(icNextSet.find(targetAddr) == icNextSet.end()){
        return;
    }

    

    string callSign = instAddr + targetAddr;
    if(countMap.find(callSign) == countMap.end()){
        countMap.insert(pair<string , int>(callSign , 1));
    }
    else{
        int& count = countMap.find(callSign)->second;
        if(count > 100){
            return;
        }
        else{
            count = count + 1;
        }
    }


    uint64_t mem_val1;
    int ret_size1 = PIN_SafeCopy(&mem_val1,(const void *)arg1,8);
    string t1 = "u";
    if(arg1 != 0)
    {
        if(ret_size1 > 0)
        {
            uint64_t tmp;
            if(mem_val1 == 0)
            {
                t1  = "p.u";
            }
            else if(PIN_SafeCopy(&tmp,(const void *)mem_val1,8) > 0)
            {
                t1 = "p.p";
            }
            else
            {
                t1 = "p.i";
            }
        }
        else if(ret_size1 == 0)
        {
            t1 = "i";
        }
        else
        {
            cerr<<"wtf?"<<endl;
        }
    }
    PIN_GetLock(&pinLock , tid + 1);
    TraceFileRetType<<std::hex<<instAddr<<'\t'<<targetAddr<<'\t'<<t1<<'\n';
    PIN_ReleaseLock(&pinLock);

}




// 插桩所有执行过的icall
VOID Ins(INS ins, VOID *v)
{
    //只插桩当前elf中的代码段，不插桩动态链接库和内核
    if(INS_Address(ins) > 0xffffff)
    {
        return;
    }
    
    // ins is icall
    if (INS_IsCall(ins) && INS_IsIndirectControlFlow(ins))
    {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(PrintIcallArgs),  IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_REG_VALUE, REG_RDI,IARG_REG_VALUE, REG_RSI,IARG_REG_VALUE, REG_RDX,IARG_REG_VALUE, REG_RCX,IARG_REG_VALUE, REG_R8,IARG_REG_VALUE, REG_R9, IARG_THREAD_ID ,IARG_END);     
    }

    if ( INS_IsRet(ins) && (!INS_IsIRet(ins)))
    {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(PrintRetArg),  IARG_INST_PTR , IARG_BRANCH_TARGET_ADDR, IARG_REG_VALUE, REG_RAX, IARG_THREAD_ID ,IARG_END);     
    }
}

/* ===================================================================== */

VOID Fini(INT32 code, VOID *v)
{
    TraceFileCfg.close();   
    TraceFileType.close();   
    TraceFileRetType.close();   
}

/* ===================================================================== */

int main(int argc, char *argv[])
{
    PIN_InitSymbols();
    PIN_Init(argc, argv);
    PIN_InitLock(& pinLock);

    string home = string(getenv("HOME"));

    filename = KnobOutputFile.Value();
    fstream file;
    cout<<filename<<endl;
    file.open((home + "/TypeSqueezer-master/TypeSqueezer/misc/dump2/" + filename + ".dump2").c_str());
    assert(file.is_open());
    string line;
    while(getline(file , line , '\n')){
        icNextSet.insert(line.substr(0 , 6));
    }
    file.close();
    TraceFileCfg.open((home + "/TypeSqueezer-master/TypeSqueezer/dynamicflow/" + filename + "_cfg.txt").c_str() , ios::app);
    TraceFileType.open((home + "/TypeSqueezer-master/TypeSqueezer/dynamictype/" + filename + "_type.txt").c_str() , ios::app);
    TraceFileRetType.open((home + "/TypeSqueezer-master/TypeSqueezer/misc/ret1/" + filename + "_ret1.txt").c_str() , ios::app);


    // TraceFile.open(argv[5]);
    // TraceFile << hex;
    // TraceFile.setf(ios::showbase);

    INS_AddInstrumentFunction(Ins, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
