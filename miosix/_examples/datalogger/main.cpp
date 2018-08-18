
#include <cstdio>
#include <miosix.h>
#include "Logger.h"
#include "ExampleData.h"

using namespace std;
using namespace miosix;

volatile bool stop=false;

void loggerDemo(void*)
{
    /*
     * Logger is configured as:
     * maxRecordSize    = 128
     * numRecords       = 128
     * bufferSize       = 4096
     * numBuffers       = 4
     * 
     * Serialized ExampleData is 30 bytes
     * There are 128 entries in the record queue, and (4-1)=3 4096 buffers for
     * buffering (the fourth buffer is the one being written).
     * Thus, the buffering system can hold 4096*3/30+128=537 ExampleData before
     * filling. Considering the rule of thumb that a high quality SD card may
     * block for up to 1s, the maximum data rate is 537Hz.
     * 
     * An estimate of the memory occupied by the logger is:
     * buffers       4*(4096+4)+128*(128+4)=33296
     * thread stacks 1536+1536+2048=5120
     * so a total of 38KB. The actual memory occupied will be a bit larger
     * due to unaccounted variables and overheads.
     * 
     * Note: although this demo is simple, the logger allows to:
     * - log data from multiple threads while being nonblocking
     * - log different classes/structs in any order, provided their serialized
     *   size is less than maxRecordSize and that they meet the requirements
     *   to be serialized with tscpp (github.com/fedetft/tscpp)
     */
    auto& logger=Logger::instance();
    logger.start();
    
    int a=0,b=0;
    auto period=2; //2ms, 500Hz
    for(auto t=getTick();;)
    {
        if(stop) break;
        auto tNew=t+period;
        Thread::sleepUntil(tNew);
        t=getTick();
        if(t>tNew) b++; //Deadline miss
        ExampleData ed(a++,b,t);
        logger.log(ed);
    }
    
    logger.stop();
    
    iprintf("Lost %d samples, missed %d deadlines\n",
            logger.getStats().statDroppedSamples,b);
}

int main()
{
    puts("Type enter to start logger demo");
    getchar();
    auto t=Thread::create(loggerDemo,2048,PRIORITY_MAX-1,nullptr,Thread::JOINABLE);
    puts("Type enter to stop logger demo");
    getchar();
    stop=true;
    t->join();
    puts("Bye");
}
