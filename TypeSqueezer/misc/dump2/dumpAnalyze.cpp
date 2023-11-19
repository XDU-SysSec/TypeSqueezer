#include<stdio.h>
#include<map>
#include<set>
#include<iostream>
#include<fstream>

using namespace std;

string filename;



int main(int argc , char* argv[]){

    
    string fileName = string(argv[1]);
    fstream fileDump;
    fileDump.open(("../../dump/" + fileName + ".dump"));
    string line;
    int i = 1;
    int flag = 0;
    fstream Tracefile;
    Tracefile.open((fileName + ".dump2"),ios::app);
    while(getline(fileDump , line , '\n')){
        if(line.size() < 10){
            continue;
        }
        if(line[8] == ':'){
            string addr = line.substr(2 , 6);
            if(flag == 1){
                Tracefile<<addr<<endl;
                flag = 0;
            }
            if(line.find("callq  *") != -1){
                flag = 1;
            }
            i = i + 1;
            
        }
    }
}
