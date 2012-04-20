
#include <map>

#ifndef TEST_ALLOC
#include <miosix.h>
#else //TEST_ALLOC
#include <iostream>
#include <typeinfo>
#include <sstream>
#endif //TEST_ALLOC

#ifndef PROCESS_POOL
#define PROCESS_POOL

/**
 * This class allows to handle a memory area reserved for the allocation of
 * processes' images. This memory area is called process pool.
 */
class ProcessPool
{
public:
    /**
     * Constructor.
     * \param poolBase address of the start of the process pool.
     * \param poolSize size of the process pool. Must be a multiple of blockSize
     */
    ProcessPool(unsigned int *poolBase, unsigned int poolSize);
    
    /**
     * Allocate memory inside the process pool.
     * \param size size of the requested memory, must be a power of two,
     * and be grater or equal to blockSize.
     * \return a pointer to the allocated memory. Note that the pointer is
     * size-aligned, so that for example if a 16KByte block is requested,
     * the returned pointer is aligned on a 16KB boundary. This is so to
     * allow using the MPU of the Cortex-M3.
     * \throws runtime_error in case the requested allocation is invalid,
     * or bad_alloc if out of memory
     */
    unsigned int *allocate(int size);
    
    /**
     * Deallocate a memory block.
     * \param ptr pointer to deallocate.
     * \throws runtime_error if the pointer is invalid
     */
    void deallocate(unsigned int *ptr);
    
    #ifdef TEST_ALLOC
    /**
     * Print the state of the allocator, used for debugging
     */
    void printAllocatedBlocks()
    {
        using namespace std;
        map<unsigned int*, unsigned int>::iterator it;
        cout<<endl;
        for(it=allocatedBlocks.begin();it!=allocatedBlocks.end();it++)
            cout <<"block of size " << it->second
                 << " allocated @ " << it->first<<endl;
        
        cout<<"Bitmap:"<<endl;
        const int SHIFT = 8 * sizeof(unsigned int);
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
    
    /**
     * Destructor
     */
    ~ProcessPool();
    
    ///This constant specifies the size of the minimum allocatable block,
    ///in bits. So for example 10 is 1KB.
    static const int blockBits=10;
    ///This constant is the the size of the minimum allocatable block, in bytes.
    static const int blockSize=1<<blockBits;
    
private:
    ProcessPool(const ProcessPool&);
    ProcessPool& operator= (const ProcessPool&);
    
    /**
     * \param bit bit to test, from 0 to poolSize/blockSize
     * \return true if the bit is set
     */
    bool testBit(int bit)
    {
        return (bitmap[bit/(sizeof(unsigned int)*8)] &
            1<<(bit % (sizeof(unsigned int)*8))) ? true : false;
    }
    
    /**
     * \param bit bit to set, from 0 to poolSize/blockSize
     */
    void setBit(int bit)
    {
        bitmap[(bit/(sizeof(unsigned int)*8))] |= 
            1<<(bit % (sizeof(unsigned int)*8));
    }
    
    /**
     * \param bit bit to clear, from 0 to poolSize/blockSize
     */
    void clearBit(int bit)
    {
        bitmap[bit/(sizeof(unsigned int)*8)] &= 
            ~(1<<(bit % (sizeof(unsigned int)*8)));
    }
    
    unsigned int *bitmap;   ///< Pointer to the status of the allocator
    unsigned int *poolBase; ///< Base address of the entire pool
    unsigned int poolSize;  ///< Size of the pool, in bytes
    ///Lists all allocated blocks, allows to retrieve their sizes
    std::map<unsigned int*,unsigned int> allocatedBlocks;
    #ifndef TEST_ALLOC
    miosix::FastMutex mutex; ///< Mutex to guard concurrent access
    #endif //TEST_ALLOC
};

#endif //PROCESS_POOL
