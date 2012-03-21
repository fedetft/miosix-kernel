/* 
 * File:   main.cpp
 * Author: lr
 *
 * Created on March 21, 2012, 2:16 PM
 */

#include <iostream>
#include "strip.h"

using namespace std;

int main(int argc, char *argv[])
{
    int stackSize=-1;
    int ramSize=-1;
    string prog;
    bool strip=false;
    for(int i=1;i<argc;i++)
    {
        string opt(argv[i]);
        if(opt=="--strip-sectheader")
            strip=true;
        else if(opt.substr(0,10)=="--ramsize=")
            ramSize=atoi(opt.substr(10).c_str());
        else if(opt.substr(0,12)=="--stacksize=")
            stackSize=atoi(opt.substr(12).c_str());
        else
            prog=opt;
    }
    if(stackSize<=0 || ramSize<=0 || prog=="")
    {
        cerr<<"usage:"<<endl<<"mxstrip prog --ramsize=<size> --stacksize=<size>"
            <<" [--strip-sectheader]"<<endl;
        return 1;
    }
    Strip s(prog);
    if(strip) s.removeSectionHeaders();
    s.setMxTags(stackSize,ramSize);
    s.writeFile();
    return 0;
}
