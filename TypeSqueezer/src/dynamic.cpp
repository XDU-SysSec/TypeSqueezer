#include<iostream>
#include<vector>
#include<string>
#include<map>
#include<fstream>
#include<cassert>

using namespace std;

#include"main.h"
#include"dynamic.h"

static map<int , set<string>> myMap;
static map<string , int> reverseMap;



void initialDynFlow(){
    ifstream file;
    file.open(dynFlowFile);
    assert(file.is_open());
    string line;
    while(getline(file , line , '\n')){
        if(line.length() != 42){
            continue;
        }
        string icaddr = line.substr(14 , 6);
        string ataddr = line.substr(36 , 6);
        if(callsiteMap.find(icaddr) == callsiteMap.end()){
            continue;
        }
        if(calleeMap.find(ataddr) == calleeMap.end()){
            continue;
        }
        set<string> s;
        s.insert(icaddr);
        s.insert(ataddr);
        if(s.empty() == false){
            Insert(s);
        } 
    }
    file.close();

}



void Insert(set<string> s){
    set<int> sets;
    int index = 100000;

    for(auto it : s){
        if(reverseMap.find(it) != reverseMap.end()){
            index = min(reverseMap[it] , index);
        }
    }
    if(index == 100000){
        int i = myMap.size() + 1;
        for(auto it : s){
            reverseMap.insert(pair<string , int>(it , i));
        }
        myMap.insert(pair<int , set<string>>(i , s));
    }else{
        for(auto it : s){
            if(reverseMap.find(it) == reverseMap.end()){
                reverseMap.insert(pair<string , int>(it , index));
                myMap[index].insert(it);
            }
        }
        for(auto it : sets){
            if(myMap.find(it) != myMap.end()){
                for(auto it2 : myMap[it]){
                    myMap[index].insert(it2);
                    reverseMap[it2] = index;
                }
                myMap.erase(it);
            }
        }
    }
}


void dynamicFlowConstrain(){


   for(auto iter : myMap){
        set<string> s = iter.second;
        int maxnum = 6;
        int minnum = 0;
        int maxlen[6] = {8 , 8 , 8 , 8 , 8 , 8};
        int minlen[6] = {0 , 0 , 0 , 0 , 0 , 0};
        typeenum type[6] = {U , U , U , U , U , U};
        int maxretnum = 1;
        int minretnum = 0;
        int maxretlen = 8;
        int minretlen = 0;
        typeenum rettype = U;


        for(auto iter2 : s){
            if(callsiteMap.find(iter2) != callsiteMap.end()){
                calliter& it = callsiteMap.find(iter2)->second;
                maxnum = min(maxnum , it.maxnum);
                minnum = max(minnum , it.minnum);
                for(int i = 0 ; i < 6 ; i++){
                    maxlen[i] = min(maxlen[i] , it.maxlen[i]);
                    minlen[i] = max(minlen[i] , it.minlen[i]);
                    type[i] = typeCombine(type[i] , it.type[i]);
                }
                maxretnum = min(maxretnum , it.maxretnum);
                minretnum = max(minretnum , it.minretnum);
                maxretlen = min(maxretlen , it.maxretlen);
                minretlen = max(minretlen , it.minretlen);
                rettype = typeCombine(rettype , it.rettype);
            }
            if(calleeMap.find(iter2) != calleeMap.end()){
                callee& it = calleeMap.find(iter2)->second;
                if(it.isvar == true) continue;
                maxnum = min(maxnum , it.maxnum);
                minnum = max(minnum , it.minnum);
                for(int i = 0 ; i < 6 ; i++){
                    maxlen[i] = min(maxlen[i] , it.maxlen[i]);
                    minlen[i] = max(minlen[i] , it.minlen[i]);
                    type[i] = typeCombine(type[i] , it.type[i]);
                }
                maxretnum = min(maxretnum , it.maxretnum);
                minretnum = max(minretnum , it.minretnum);
                maxretlen = min(maxretlen , it.maxretlen);
                minretlen = max(minretlen , it.minretlen);
                rettype = typeCombine(rettype , it.rettype);
            }
        }
        if((minnum > maxnum) || (minretnum > maxretnum)){
            continue;
        }
        int flag = 0;
        for(int i = 0 ; i < minnum ; i++){
            if(minlen[i] > maxlen[i]){
                flag = 1;
            }
        }
        if(flag == 1) continue;
        if(minretnum == 1){
            if(minretlen > maxretlen){
            continue;
            }
        }

        for(auto iter2 : s){
            if(callsiteMap.find(iter2) != callsiteMap.end()){
                callsite& it = callsiteMap.find(iter2)->second;
                it.maxnum = maxnum;
                it.minnum = minnum;
                for(int i = 0 ; i < 6 ; i++){
                    it.maxlen[i] = maxlen[i];
                    it.minlen[i] = minlen[i];
                    it.type[i] = type[i];
                }
                it.maxretnum = maxretnum;
                it.minretnum = minretnum;
                it.maxretlen = maxretlen;
                it.minretlen = minretlen;
                it.rettype = rettype;
            }
            if(calleeMap.find(iter2) != calleeMap.end()){
                callee& it = calleeMap.find(iter2)->second;
                if(it.isvar == true) continue;
                it.maxnum = maxnum;
                it.minnum = minnum;
                for(int i = 0 ; i < 6 ; i++){
                    it.maxlen[i] = maxlen[i];
                    it.minlen[i] = minlen[i];
                    it.type[i] = type[i];
                }
                it.maxretnum = maxretnum;
                it.minretnum = minretnum;
                it.maxretlen = maxretlen;
                it.minretlen = minretlen;
                it.rettype = rettype;
            }
        }
   }

}


typeenum typeCombine(typeenum t1 , typeenum t2){
    if(t1 == t2) return t1;
    if(t1 == U) return t2;
    if(t2 == U) return t1;
    if((t1 == I)||(t2 == I)) return I;
    if(t1 == PU) return t2;
    if(t2 == PU) return t1;
    if((t1 == PI)||(t2 == PI)) return PI;
    return t1;
}

void initialDynType(){
    initialDynTypeFlow();
    initialDynTypeRet();
}

void initialDynTypeFlow(){
    ifstream file;
    file.open(dynTypeFlowFile);
    assert(file.is_open());
    string line;
    while(getline(file , line , '\n')){
        if((line.length() > 14) && (line[13] == '\t')){
            string icAddr = line.substr(0 , 6);
            string atAddr = line.substr(7 , 6);
            if(callsiteMap.find(icAddr) == callsiteMap.end()){
                continue;
            }
            if(calleeMap.find(atAddr) == calleeMap.end()){
                continue;
            }
            if(callsiteMap.find(icAddr) != callsiteMap.end()){
                typeenum enum_t;
                callsite& iter = callsiteMap.find(icAddr)->second;
                int a = 14;
                for(int i = 0 ; i < 5 ; i++){
                    string t = line.substr(a , line.find('\t' , a + 1) - a);

                    enum_t = U;
                    if(t == "u") {enum_t = U;}
                    else if(t == "i") {enum_t = I;}
                    else if(t == "p.u") {enum_t = PU;}
                    else if(t == "p.i") {enum_t = PI;}
                    else if(t == "p.p") {enum_t = PP;}
                    else {
                    // cout<<a<<endl;
                    // cout<<t<<endl;
                    assert(1);
                    }                
                    iter.type[i] = typeCombine(iter.type[i] , enum_t);
                    a = line.find('\t' , a + 1) + 1;
                }
                string t = line.substr(a , line.size() - a);
                enum_t = U;
                if(t == "u") {enum_t = U;}
                else if(t == "i") {enum_t = I;}
                else if(t == "p.u") {enum_t = PU;}
                else if(t == "p.i") {enum_t = PI;}
                else if(t == "p.p") {enum_t = PP;}
                else {
                    // cout<<a<<endl;
                    // cout<<t<<endl;
                    assert(1);
                }   
                iter.type[5] = typeCombine(iter.type[5] , enum_t);
            }



            if(calleeMap.find(atAddr) != calleeMap.end()){
                typeenum enum_t;
                callee& iter = calleeMap.find(atAddr)->second;
                int a = 14;
                for(int i = 0 ; i < 5 ; i++){
                    string t = line.substr(a , line.find('\t' , a + 1) - a);

                    enum_t = U;
                    if(t == "u") {enum_t = U;}
                    else if(t == "i") {enum_t = I;}
                    else if(t == "p.u") {enum_t = PU;}
                    else if(t == "p.i") {enum_t = PI;}
                    else if(t == "p.p") {enum_t = PP;}
                    else {
                    // cout<<a<<endl;
                    // cout<<t<<endl;
                    assert(1);
                    }                
                    iter.type[i] = typeCombine(iter.type[i] , enum_t);
                    a = line.find('\t' , a + 1) + 1;
                }
                string t = line.substr(a , line.size() - a);
                enum_t = U;
                if(t == "u") {enum_t = U;}
                else if(t == "i") {enum_t = I;}
                else if(t == "p.u") {enum_t = PU;}
                else if(t == "p.i") {enum_t = PI;}
                else if(t == "p.p") {enum_t = PP;}
                else {
                    // cout<<a<<endl;
                    // cout<<t<<endl;
                    assert(1);
                }   
                iter.type[5] = typeCombine(iter.type[5] , enum_t);
            }
        }
    }
    file.close();
}



void initialDynTypeRet(){
    ifstream file;
    file.open(dynTypeRetFile);
    assert(file.is_open());
    string line;                        
    while(getline(file , line , '\n')){
        // if(line.length() > 19) line = line.substr(5 , line.size() - 5);
        if(line.length() >= 19){
            string icAddr = line.substr(2 , 6);
            string atAddr = line.substr(11 , 6);
            if(callsiteMap.find(icAddr) != callsiteMap.end()){
                callsite& iter = callsiteMap.find(icAddr)->second;
                string t_str = line.substr(18 , line.length() - 18);
                typeenum t_enum = U;
                if(t_str == "u") {t_enum = U;}
                else if(t_str == "i") {t_enum = I;}
                else if(t_str == "p.u") {t_enum = PU;}
                else if(t_str == "p.i") {t_enum = PI;}
                else if(t_str == "p.p") {t_enum = PP;}
                else {
                    assert(0);
                }
                iter.rettype = typeCombine(iter.rettype , t_enum);
            }
        }       
    }
    file.close();
}