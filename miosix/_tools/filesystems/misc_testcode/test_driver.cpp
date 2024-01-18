// Compile with g++ -g -o t test_driver.cpp -std=c++0x -m32

#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <climits>
#include <memory>
#include <errno.h>
#include <sys/stat.h>

using namespace std;

///////////////////////// Stubs ///////////////////////////

#define intrusive_ref_ptr shared_ptr

class StringPart;

class FilesystemBase
{
public:
    FilesystemBase() {}
    virtual int lstat(StringPart& name, struct stat *pstat);
    virtual int readlink(StringPart& name, std::string& target);
    virtual bool supportsSymlinks() { return true; }
    virtual ~FilesystemBase() {}
private:
    FilesystemBase(const FilesystemBase&);
    FilesystemBase& operator= (const FilesystemBase&);
};

///////////////////////// Code under test /////////////////

//Insert classes StringPart, ResolvedPAth and PathResolution

////////////////// end of test code //////////////////////////

int FilesystemBase::lstat(StringPart& name, struct stat *pstat)
{
    if(string(name.c_str())=="home/link") pstat->st_mode=0120777;
    else if(string(name.c_str())=="home/link2") pstat->st_mode=0120777;
    else {
        if(string(name.c_str()).find("file")!=string::npos)
            pstat->st_mode=0775;
        else
            pstat->st_mode=040777;
    }
    return 0;
}

int FilesystemBase::readlink(StringPart& name, std::string& target)
{
    if(string(name.c_str())=="home/link")
    {
        target="../dev/null";
        return 0;
    } else if(string(name.c_str())=="home/link2") {
        target="/home/test";
        return 0;
    } else return -ENOENT;
}

int main(int argc, char *argv[])
{
	map<StringPart,intrusive_ref_ptr<FilesystemBase>> fs;
	intrusive_ref_ptr<FilesystemBase> root(new FilesystemBase);
    //cout<<"----------"<<endl;
	fs.insert(make_pair(StringPart("/"),root));
	fs.insert(make_pair(StringPart("/dev"),intrusive_ref_ptr<FilesystemBase>(new FilesystemBase)));
    //cout<<"----------"<<endl;
	PathResolution pr(fs);
	string path=argv[1];
	ResolvedPath rp=pr.resolvePath(path,true);
	if(rp.result==0)
	{
		assert((unsigned)rp.off<=path.length());
		string p2=path;
		StringPart sp(path,string::npos,rp.off);
		cout<<"resolved path="<<p2
			<<" fs="<< (rp.fs==root ? "/" : "/dev")
			<<" offset="<<rp.off
			<<" fs-relative path="<<sp.c_str()<<endl;
	} else {
		errno=-rp.result;
		perror("path resolution");
	}
}
