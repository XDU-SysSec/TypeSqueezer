#include<iostream>
#include<iostream>
#include<vector>
#include<string>
#include<map>
#include<fstream>
#include<cassert>

using namespace std;

#include "main.h"
#include "initial.h"
#include "output.h"
#include "direct.h"
#include "dynamic.h"

string binFile;
string atFile;
string typeArmorFile;
string tauCfiIcFile;
string tauCfiIcRetFile;
string tauCfiAtRetFile;
string tauCfiAtFile;
string unreachFile;
string dynFlowFile;
string optFile;

string directCallTypeArmorCallFile;
string directCallTypeArmorRetFile;
string directCallTauCfiCallFile;
string directCallTauCfiRetFile;
string dynTypeFlowFile;
string dynTypeRetFile;

string outputFile;


map<string , string> atMap;
map<string , callee> taCalleeMap;
set<string> taSet;
map<string , callsite> callsiteMap;
map<string , callee> calleeMap;

calliter::calliter(string addr , string name) : addr(addr) , name(name){
    this->maxnum = 6;
    this->minnum = 0;
    for(int i = 0 ; i < 6 ; i++){
        this->minlen[i] = 0;
        this->maxlen[i] = 8;
        this->type[i] = U;
    }
    this->maxretnum = 1;
    this->minretnum = 0;
    this->minretlen = 0;
    this->maxretlen = 8;
    this->rettype = U;
}

calliter::~calliter(){}

callsite::callsite(string addr , string name) : calliter(addr , name){
    this->unreachable = false;
}

callee::callee(string addr , string name) : calliter(addr , name){
    this->maxretnum = 0;
    this->isat = true;
    this->isvar = false;
}

int main(int argc , char* argv[]){
    assert(argc == 2);
    string binName(argv[1]);
    initial(binName);
    initialDynFlow();
    initialDynType();
    dynamicFlowConstrain();
    directCallTypeArmor();
    directCallTauCfi();
    dynamicFlowConstrain();
    outputResFstream();

    
}
