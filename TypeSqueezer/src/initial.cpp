#include<iostream>
#include<vector>
#include<string>
#include<map>
#include<fstream>
#include<cassert>

using namespace std;

#include"main.h"
#include"initial.h"
#include"output.h"

void initial(string bin){
    cout<<"initialfile"<<endl;
    initialFile(bin);
    cout<<"initialat"<<endl;
    initialAT();
    cout<<"initialstatic"<<endl;
    initialStatic();
}

static set<string> compiler_func = {"deregister_tm_clones",
"register_tm_clones",
"__do_global_dtors_aux",
"frame_dummy",
"__libc_csu_init",
"__libc_csu_fini",
"_fini",
"__frame_dummy_init_array_entry",
"__init_array_start",
"__do_global_dtors_aux_fini_array_entry",
"__init_array_end",
"_start",
"_init",
"_GLOBAL__sub_I_way"};

static set<string> compiler_addr;


void initialFile(string bin){
    atFile = "../at/" + bin + ".at";
    optFile = "../FSROPT-out/binfo." + bin;
    dynFlowFile = "../dynamicflow/" + bin + "_cfg.txt";
    tauCfiIcFile = "../taucfi/" + bin + "_icall";
    tauCfiIcRetFile = "../taucfi/" + bin + "_icall_ret";
    tauCfiAtRetFile = "../taucfi/" + bin + "_AT_ret";
    tauCfiAtFile = "../taucfi/" + bin + "_AT_funcs";

    outputFile = "../res/" + bin + ".res";

    directCallTypeArmorCallFile = "../directcall/Dcall_TypeArmor/" + bin + "_dcall";
    directCallTypeArmorRetFile = "../directcall/Dcall_TypeArmor/" + bin + "_dcall_ret";
    directCallTauCfiCallFile = "../directcall/Dcall_tauCFI/" + bin + "_dcall";
    directCallTauCfiRetFile = "../directcall/Dcall_tauCFI/" + bin + "_dcall_ret";
    dynTypeFlowFile = "../dynamictype/" + bin + "_type.txt";
    dynTypeRetFile = "../ret/" + bin + ".ret";
}


void initialAT(){
    ifstream file;
    file.open(atFile);
    assert(file.is_open());
    string line;
    while(getline(file , line , '\n')){
        string addr = line.substr(0 , 6);
        string name = line.substr(9 , line.size() - 9);
        atMap.insert(pair<string , string>(addr , name));
    }
    file.close();
}




void initialStatic(){
    fstream file;
    file.open(optFile);
    assert(file.is_open());
    string line;
    while(getline(file , line , '\n')){
        if(line.find("[varargs]") != -1){
            getline(file , line , '\n');
            while((line.length() >= 12) && (line.find("(") != -1)){
                string addr = line.substr(2 , 6);
                string name = line.substr(14 , line.find(')') - 14);
                if(compiler_func.count(name) != 0){
                    compiler_addr.insert(addr);
                    getline(file , line , '\n');
                    continue;
                }
                if(atMap.find(addr) != atMap.end()){
                    callee* iter = new callee(addr , name);
                    iter->addr = line[11] - '0';
                    iter->isvar = true;
                    calleeMap.insert(pair<string , callee>(addr , *iter));
                }
                getline(file , line , '\n');
            }
        }
        if(line.find("[args]") != -1){
            getline(file , line , '\n');
            while((line.length() >= 12) && (line.find("(") != -1)){
                string addr = line.substr(2 , 6);
                string name = line.substr(14 , line.find(')') - 14);
                if(compiler_func.count(name) != 0){
                    getline(file , line , '\n');
                    compiler_addr.insert(addr);
                    continue;
                }
                taSet.insert(addr);
                if(atMap.find(addr) != atMap.end()){
                    callee* iter = new callee(addr , name);
                    iter->minnum = line[11] - '0';
                    int index = line.find(')') + 2;
                    for(int i = 0 ; i < iter->minnum ; i++){
                        assert((line[index] >= '0') && (line[index] <= '9'));
                        if(line[index] != '9'){
                            iter->minlen[i] = line[index] - '0';
                            index += 2;
                        }
                    }
                    calleeMap.insert(pair<string , callee>(addr , *iter));
                }
                callee* iter = new callee(addr , name);
                iter->minnum = line[11] - '0';
                int index = line.find(')') + 2;
                for(int i = 0 ; i < iter->minnum ; i++){
                    assert((line[index] >= '0') && (line[index] <= '9'));
                    if(line[index] != '9'){
                        iter->minlen[i] = line[index] - '0';
                        index += 2;
                    }
                }
                taCalleeMap.insert(pair<string , callee>(addr , *iter));
                getline(file , line , '\n');
            }
        }
        if(line.find("[icall-args]") != -1){
            getline(file , line , '\n');
            while((line.length() >= 12) && (line.find("(") != -1)){
                string addr = line.substr(2 , 6);
                string name = line.substr(14 , line.find_last_of('.') - 14);
                if(compiler_func.count(name) != 0){
                    compiler_addr.insert(addr);
                    getline(file , line , '\n');
                    continue;
                }
                int index = line.find(')') + 2;
                callsite* iter = new callsite(addr , name);
                iter->maxnum = line[11] - '0';
                for(int i = 0 ; i < iter->maxnum ; i++){
                    assert((line[index] >= '0') && (line[index] <= '9'));
                    if(line[index] != '9'){
                        iter->maxlen[i] = line[index] - '0';
                        index += 2;
                    }
                }
                callsiteMap.insert(pair<string , callsite>(addr , *iter));
                getline(file , line , '\n');
            }
        }
        if(line.find("[non-voids]") != -1){
            getline(file , line , '\n');
            while(line.length() >= 12){
                string addr = line.substr(2 , 6);
                if(calleeMap.find(addr) != calleeMap.end()){
                    callee& iter = calleeMap.find(addr)->second;
                    iter.maxretnum = 1;
                }
                getline(file , line , '\n');
            }
        }
        if(line.find("[non-void-icalls]") != -1){
            getline(file , line , '\n');
            while(line.length() >= 12){
                string addr = line.substr(2 , 6);
                if(callsiteMap.find(addr) != callsiteMap.end()){
                    callsite& iter = callsiteMap.find(addr)->second;
                    iter.minretnum = 1;
                }
                getline(file , line , '\n');
            }
        }
    }

    file.close();

    file.open(tauCfiIcRetFile);
    assert(file.is_open());
    while(getline(file , line , '\n')){
        if((line.length() <= 9) && (line.length() >= 8)){
            string addr = line.substr(0 , 6);
            string len = line.substr(7 , line.length() - 7);
            if(callsiteMap.find(addr) != callsiteMap.end()){
                callsite& ic = callsiteMap.find(addr)->second;
                if((len != "0") && (len != "-1")){
                    ic.minretlen = atoi(len.c_str());
                }
            }
        }
    }
    file.close();

    file.open(tauCfiAtRetFile);
    assert(file.is_open());
    while(getline(file , line , '\n')){
        if(line.length() == 8){
            string addr = line.substr(0 , 6);
            if(calleeMap.find(addr) != calleeMap.end()){
                callee& at = calleeMap.find(addr)->second;
                int len = line[7] - '0';
                if(len != 0){
                    at.maxretlen = len;
                }
            }
        }
    }
    file.close();







}