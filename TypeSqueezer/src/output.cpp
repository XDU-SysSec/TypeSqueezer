#include<iostream>
#include<vector>
#include<string>
#include<map>
#include<fstream>
#include<cassert>

using namespace std;

#include "main.h"
#include "output.h"


void outputResFstream(){
    ofstream file;
    file.open(outputFile);
    assert(file.is_open());
    file<<"[args]"<<endl;
    for(auto it : calleeMap){
        if(it.second.isvar != true){
            file<<it.second.addr<<'\t'<<it.second.name<<'\t';
            file<<"num:("<<it.second.minnum<<it.second.maxnum<<")";
            file<<"rnum:("<<it.second.minretnum<<it.second.maxretnum<<")\t";
            file<<"wid:(";
            for(int i = 0 ; i < it.second.maxnum ; i++){
                file<<'['<<it.second.minlen[i]<<it.second.maxlen[i]<<']';
            }
            file<<")\trwid:(";
            for(int i = 0 ; i < it.second.maxretnum ; i++){
                file<<'['<<it.second.minlen[i]<<it.second.maxlen[i]<<']';
            }
            file<<")\ttype:(";
            for(int i = 0 ; i < it.second.maxnum ; i++){
                file<<'[';
                if(it.second.type[i] == U) file<<"u";
                if(it.second.type[i] == PI) file<<"pi";
                if(it.second.type[i] == PU) file<<"pu";
                if(it.second.type[i] == I) file<<"i";
                if(it.second.type[i] == PP) file<<"pp";
                file<<']';
            }
            file<<")\trtype:(";
            for(int i = 0 ; i < it.second.maxretnum ; i++){
                file<<'[';
                if(it.second.rettype == U) file<<"u";
                if(it.second.rettype == PI) file<<"pi";
                if(it.second.rettype == PU) file<<"pu";
                if(it.second.rettype == I) file<<"i";
                if(it.second.rettype == PP) file<<"pp";
                file<<']';
            }
            file<<')'<<endl;
        }
    }
    file<<endl;

    file<<"[icall]"<<endl;
    for(auto it : callsiteMap){
            file<<it.second.addr<<'\t'<<it.second.name<<'\t';
            file<<"num:("<<it.second.minnum<<it.second.maxnum<<")";
            file<<"rnum:("<<it.second.minretnum<<it.second.maxretnum<<")\t";
            file<<"wid:(";
            for(int i = 0 ; i < it.second.maxnum ; i++){
                file<<'['<<it.second.minlen[i]<<it.second.maxlen[i]<<']';
            }
            file<<")\trwid:(";
            for(int i = 0 ; i < it.second.maxretnum ; i++){
                file<<'['<<it.second.minlen[i]<<it.second.maxlen[i]<<']';
            }
            file<<")\ttype:(";
            for(int i = 0 ; i < it.second.maxnum ; i++){
                file<<'[';
                if(it.second.type[i] == U) file<<"u";
                if(it.second.type[i] == PI) file<<"pi";
                if(it.second.type[i] == PU) file<<"pu";
                if(it.second.type[i] == I) file<<"i";
                if(it.second.type[i] == PP) file<<"pp";
                file<<']';
            }
            file<<")\trtype:(";
            for(int i = 0 ; i < it.second.maxretnum ; i++){
                file<<'[';
                if(it.second.rettype == U) file<<"u";
                if(it.second.rettype == PI) file<<"pi";
                if(it.second.rettype == PU) file<<"pu";
                if(it.second.rettype == I) file<<"i";
                if(it.second.rettype == PP) file<<"pp";
                file<<']';
            }
            file<<'('<<endl;
    }
    file<<endl;

    file<<"[done]"<<endl;
    file.close();

}