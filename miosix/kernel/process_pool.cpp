
#include "process_pool.h"
#include <stdexcept>
#include <cstring>

using namespace std;

ProcessPool::ProcessPool(unsigned int *poolBase, unsigned int poolSize)
    : poolBase(poolBase), poolSize(poolSize)
{
    int numBytes=poolSize/blockSize/8;
    bitmap=new unsigned int[numBytes/sizeof(unsigned int)];
    memset(bitmap,0,numBytes);
}
    
unsigned int *ProcessPool::allocate(unsigned int size)
{
    #ifndef TEST_ALLOC
    miosix::Lock<miosix::FastMutex> l(mutex);
    #endif //TEST_ALLOC
    //If size is not a power of two, or too big or small
    if((size & (size - 1)) || size>poolSize || size<blockSize)
            throw runtime_error("");
    
    unsigned int offset=0;
    if(reinterpret_cast<unsigned int>(poolBase) % size)
        offset=size-(reinterpret_cast<unsigned int>(poolBase) % size);
    unsigned int startBit=offset/blockSize;
    unsigned int sizeBit=size/blockSize;

    for(unsigned int i=startBit;i<=poolSize/blockSize;i+=sizeBit)
    {
        bool notEmpty=false;
        for(unsigned int j=0;j<sizeBit;j++)
        {
            if(testBit(i+j)==0) continue;
            notEmpty=true;
            break;
        }
        if(notEmpty) continue;
        
        for(unsigned int j=0;j<sizeBit;j++) setBit(i+j);
        unsigned int *result=poolBase+i*blockSize/sizeof(unsigned int);
        allocatedBlocks[result]=size;
        return result;
    }
    throw bad_alloc();
}

void ProcessPool::deallocate(unsigned int *ptr)
{
    #ifndef TEST_ALLOC
    miosix::Lock<miosix::FastMutex> l(mutex);
    #endif //TEST_ALLOC
    map<unsigned int*, unsigned int>::iterator it= allocatedBlocks.find(ptr);
    if(it==allocatedBlocks.end())throw runtime_error("");
    unsigned int size =(it->second)/blockSize;
    unsigned int firstBit=(reinterpret_cast<unsigned int>(ptr)-
                           reinterpret_cast<unsigned int>(poolBase))/blockSize;
    for(unsigned int i=firstBit;i<firstBit+size;i++) clearBit(i);
    allocatedBlocks.erase(it);
}

ProcessPool::~ProcessPool()
{
    delete[] bitmap;
}

#ifdef TEST_ALLOC
int main()
{
    ProcessPool pool(reinterpret_cast<unsigned int*>(0x20008000),96*1024);
    while(1)
    {
        cout<<"a<size(exponent)>|d<addr>"<<endl;
        unsigned int param;
        char op;
        string line;
        getline(cin,line);
        stringstream ss(line);
        ss>>op;
        switch(op)
        {
            case 'a':
                ss>>dec>>param;
                try {
                    pool.allocate(1<<param);
                } catch(exception& e) {
                    cout<<typeid(e).name();
                }
                pool.printAllocatedBlocks();
                break;
            case 'd':
                ss>>hex>>param;
                try {
                    pool.deallocate(reinterpret_cast<unsigned int*>(param));
                } catch(exception& e) {
                    cout<<typeid(e).name();
                }
                pool.printAllocatedBlocks();
                break;
            default:
                cout<<"Incorrect option"<<endl;
                break;
        }          
    }
}
#endif //TEST_ALLOC
