#ifndef MAIN_H
#define MAIN_H

#include<iostream>
#include<vector>
#include<string>
#include<map>
#include<fstream>
#include<set>

using namespace std;

enum typeenum{U , I , PU , PP , PI};

class calliter{
    public:
        string addr;
        string name;
        int maxnum;
        int minnum;
        int maxlen[6];
        int minlen[6];
        typeenum type[6];
        int maxretnum;
        int minretnum;
        int maxretlen;
        int minretlen;
        typeenum rettype;
        calliter(string addr , string name);
        calliter() = default;
        virtual ~calliter();
};

class callsite : public calliter{
    public:
        bool unreachable;
        callsite(string addr , string name);
};

class callee : public calliter{
    public:
        bool isat;
        bool isvar;
        callee(string addr , string name);
};




extern string atFile;
extern string optFile;
extern string dynFlowFile;

extern string directCallTypeArmorCallFile;
extern string directCallTypeArmorRetFile;
extern string directCallTauCfiCallFile;
extern string directCallTauCfiRetFile;
extern string dynTypeFlowFile;
extern string dynTypeRetFile;

extern string tauCfiIcFile;
extern string tauCfiIcRetFile;
extern string tauCfiAtRetFile;
extern string tauCfiAtFile;

extern string outputFile;


extern set<string> taSet;
extern map<string , string> atMap;
extern map<string , callee> taCalleeMap;
extern map<string , callsite> callsiteMap;
extern map<string , callee> calleeMap;



#endif