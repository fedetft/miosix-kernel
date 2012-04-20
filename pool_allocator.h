
#include <map>

#ifndef TEST_ALLOC
#include <miosix.h>
#else //TEST_ALLOC
#include <iostream>
#include <typeinfo>
#include <sstream>
#endif //TEST_ALLOC

#ifndef POOL_ALLOCATOR
#define POOL_ALLOCATOR

class PoolAllocator
{
public:
    PoolAllocator(unsigned int *poolBase, unsigned int poolSize);
    
    unsigned int *allocate(int size);
    
    void deallocate(unsigned int *ptr);
    
    #ifdef TEST_ALLOC
    void printAllocatedBlocks()
    {
        using namespace std;
        map<unsigned int*, unsigned int>::iterator it;
        cout<<endl;
        for(it=allocatedBlocks.begin();it!=allocatedBlocks.end();it++)
            cout <<"block of size " << it->second
                 << " allocated @ " << it->first<<endl;
        
        cout<<endl<<endl<<"Bitmap:"<<endl;
        const int SHIFT = 8 * sizeof( unsigned int);
        const unsigned int MASK = 1 << (SHIFT-1);
        int bitarray[32];
        for(int i=0; i<(poolSize/blockSize)/(sizeof(unsigned int)*8);i++)
        {   
            int value=bitmap[i];
            for ( int j = 0; j < SHIFT; j++ ) 
            {
                bitarray[31-j]= ( value & MASK ? 1 : 0 );
                value <<= 1;
            }
            for(int j=0;j<32;j++)
                cout<<bitarray[j];

            cout << endl;
        }
        
        
        
    }
    #endif //TEST_ALLOC
    
    ~PoolAllocator();
    
    static const int blockBits=10; //size of the minimum allocatable block
                                   //specified in bits
    static const int blockSize=1<<blockBits;
    
private:
    PoolAllocator(const PoolAllocator&);
    PoolAllocator& operator= (const PoolAllocator&);
    
    bool testBit(int bit)
    {
        return (bitmap[bit/(sizeof(unsigned int)*8)] &
                1<<(bit % (sizeof(unsigned int)*8))) ? true : false;
    }
    
    void setBit(int bit)
    {
        bitmap[(bit/(sizeof(unsigned int)*8))] |= 
                                           1<<(bit % (sizeof(unsigned int)*8));
    }
    
    void clearBit(int bit)
    {
        bitmap[bit/(sizeof(unsigned int)*8)] &= 
                                        ~(1<<(bit % (sizeof(unsigned int)*8)));
    }
    
    unsigned int *bitmap;
    unsigned int *poolBase;
    unsigned int poolSize;
    std::map<unsigned int*,unsigned int> allocatedBlocks;
    #ifndef TEST_ALLOC
    miosix::FastMutex mutex;
    #endif //TEST_ALLOC
};

#endif //POOL_ALLOCATOR
