
#include "pool_allocator.h"

PoolAllocator::PoolAllocator(unsigned int *poolBase, unsigned int poolSize)
    : poolBase(poolBase), poolSize(poolSize)
{
    bitmap=new unsigned int[poolSize/blockSize/sizeof(unsigned int)];
}
    
unsigned int *PoolAllocator::allocate(int size)
{
    //TODO : align this, and validate size to prevent wraparound
    unsigned int offset=0;
    if(poolBase % size) offset=size-(poolBase % size);
    int startBit=offset/blockSize;
}

void PoolAllocator::deallocate(unsigned int *ptr)
{
    
}

PoolAllocator::~PoolAllocator()
{
    delete[] bitmap;
}
