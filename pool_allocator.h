
#include <map>

#ifndef POOL_ALLOCATOR
#define POOL_ALLOCATOR

class PoolAllocator
{
public:
    PoolAllocator(unsigned int *poolBase, unsigned int poolSize);
    
    unsigned int *allocate(int size);
    
    void deallocate(unsigned int *ptr);
    
    ~PoolAllocator();
    
    static const int blockSize=1024;
    
private:
    PoolAllocator(const PoolAllocator&);
    PoolAllocator& operator= (const PoolAllocator&);
    
    unsigned int *bitmap;
    unsigned int *poolBase;
    unsigned int poolSize;
    std::map<unsigned int*,unsigned int> allocatedBlocks;
};

#endif //POOL_ALLOCATOR
