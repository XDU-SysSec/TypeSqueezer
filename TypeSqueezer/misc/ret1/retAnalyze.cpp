#include<stdio.h>
#include<map>
#include<set>
#include<iostream>
#include<fstream>

using namespace std;

string fileName;
fstream file;
fstream Tracefile;
fstream file2;

map<int , string> lineIndexMap;
map<string , int> addrIndexMap;
map<string , string> atIndexMap;
map<string , string> signTypeMap;



string fun(string str1 , string str2){
    if(str1 == str2) return str2;
    if(str1 == "u") return str2;
    if(str2 == "u") return str1;
    if((str1 == "i") && (str2[0] == 'p')) return "i";
    if((str2 == "i") && (str1[0] == 'p')) return "i";
    if(str1 == "p.u") return str2;
    if(str2 == "p.u") return str1;
    if(str1 == "p.i") return "p.i";
    if(str2 == "p.i") return "p.i";
    return str2;
}



int main(int argc , char* argv[]){
    string fileName = argv[1];
    file2.open(("../../dump/" + fileName + ".dump").c_str());
    file.open((fileName + ".ret1").c_str());
    Tracefile.open(("../../ret/" + fileName + ".ret").c_str() , ios::app);
    string line;
    int i = 1;
    while(getline(file2 , line , '\n')){
        if((line.size() >= 16) && (line.substr(0 , 10) == "0000000000")){
            string addr = line.substr(10 , 6);
            getline(file2 , line , '\n');
            while((line.size() >= 8) && (line[8] == ':')){
                string addr2 = line.substr(2 , 6);
                atIndexMap.insert(pair<string , string>(addr2 , addr));
                lineIndexMap.insert(pair<int , string>(i , addr2));
                addrIndexMap.insert(pair<string , int>(addr2 , i));
                i = i + 1;
                getline(file2 , line ,'\n');
            }
        }
    }

    string line2;
    while(getline(file , line2 , '\n')){
        if(line2.size() <= 14){
            continue;
        }
        string icaddr = lineIndexMap.find(addrIndexMap.find(line2.substr(7 , 6))->second - 1)->second;
        if(atIndexMap.find(line2.substr(0 , 6)) == atIndexMap.end()){
            continue;
        }
        string ataddr = atIndexMap.find(line2.substr(0 , 6))->second;
        string sign = icaddr + ataddr;
        string type = line2.substr(14 , line2.size() - 14);
        if(signTypeMap.find(sign) == signTypeMap.end()){
            signTypeMap.insert(pair<string ,string>(sign , type));
        }
        string& type1 = signTypeMap.find(sign)->second;
        type1 = fun(type , type1);


    }

    for(auto it = signTypeMap.begin() ; it != signTypeMap.end() ; it++){
        string ic = it->first.substr(0 , 6);
        string at = it->first.substr(6 , 6);
        string type = it->second;
        Tracefile<<"ic"<<ic<<'\t'<<"at"<<at<<'\t'<<type<<endl;
    }


}
