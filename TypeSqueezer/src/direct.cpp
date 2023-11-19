#include<iostream>
#include<vector>
#include<string>
#include<map>
#include<fstream>
#include<cassert>

using namespace std;

#include"main.h"
#include"direct.h"
#include"output.h"



void directCallTypeArmor(){
    directCallTypeArmorCall();
    directCallTypeArmorRet();
}

void directCallTauCfi(){
    directCallTauCfiCall();
    directCallTauCfiRet();
}


void directCallTypeArmorCall(){
    ifstream file;
    file.open(directCallTypeArmorCallFile);
    assert(file.is_open());
    string line;
    while(getline(file , line , '\n')){
        if(line.length() == 20){
            string addr = line.substr(7 , 6);
            if(calleeMap.find(addr) != calleeMap.end()){
                int index = 0;
                for(int i = 0 ; i  < 6 ; i++){
                    if(line[14 + i] == 'W'){
                        index += 1;
                    }
                    else{
                        break;
                    }
                }
                callee& iter = calleeMap.find(addr)->second;
                if(iter.isvar == false){
                    iter.maxnum = min(iter.maxnum , index);
                }
            }
        }

    }
    file.close();
}





void directCallTypeArmorRet(){
    ifstream file;
    file.open(directCallTypeArmorCallFile);
    assert(file.is_open());
    string line;
    while(getline(file , line , '\n')){
        if(line.length() == 22){
            string addr = line.substr(7 , 6);
            if(calleeMap.find(addr) != calleeMap.end()){
                callee& iter = calleeMap.find(addr)->second;
                iter.minretlen = 1;
                if(iter.maxretnum == 0) cout<<"directmaxretnumwrong\t"<<addr<<endl;
            }

        }
    }
    file.close();
}




void directCallTauCfiCall(){
    ifstream file;
    file.open(directCallTauCfiCallFile);
    assert(file.is_open());
    string line;
    while(getline(file , line , '\n')){
        if(line.length() == 20){
            string addr = line.substr(7 , 6);
            if(calleeMap.find(addr) != calleeMap.end()){
                callee& iter = calleeMap.find(addr)->second;
                for(int i = 0 ; i < 6 ; i++){
                    if(line[14 + i] != '0'){
                        iter.maxlen[i] = min(iter.maxlen[i] , (line[14 + i] - '0'));
                    }
                }
            }
        }
    }
    file.close();
}



void directCallTauCfiRet(){
    ifstream file;
    file.open(directCallTauCfiRetFile);
    assert(file.is_open());
    string line;
    while(getline(file , line , '\n')){
        if(line.length() == 15){
            string addr = line.substr(7 , 6);
            if(calleeMap.find(addr) != calleeMap.end()){
                callee& iter = calleeMap.find(addr)->second;
                if(line[14] != '0'){
                    iter.minretlen = max(iter.minretlen , line[14] - '0');
                }
            }
        }
    }
    file.close();
}


