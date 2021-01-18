
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <list>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <miosix.h>

using namespace std;
using namespace std::chrono;
using namespace miosix;

/**
 * This struct contains sensor data
 */
struct Record
{
    int data[15]; //TODO: put fields representing sensor data
    unsigned int timestamp;
};

/**
 * Perform sensor reading
 * \return a record with the sensor data
 */
Record readSensors()
{
    Record result;
    //TODO: insert sensor reading code here instead of memset()
    memset(&result,0,sizeof(result));
    result.timestamp=duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
    return result;
}

/**
 * Process sensor readings
 */
void processSensorData(const Record& sample)
{
    //TODO: perform sensor filtering and decision making
}

const int fifoSize=48*1024;   ///< Size of FIFO
const unsigned int bufferSize=32*1024; ///< Size of buffer
const int recordPerBuffer=bufferSize/sizeof(Record);
const int numBuffers=2;       ///< Number of buffers

/// This is a FIFO buffer between senseThread() and packThread()
/// It is a global variable so it ends up in the 64KB CCM RAM of the stm32f4
static Queue<Record,fifoSize/sizeof(Record)> queuedSamples;

static list<Record *> fullList;       ///< Buffers between packThread() and writeThread()
static list<Record *> emptyList;      ///< Buffers between packThread() and writeThread()
static FastMutex listHandlingMutex;   ///< To allow concurrent access to the lists
static ConditionVariable listWaiting; ///< To lock when buffers are all full/empty

static Thread *statsT;           ///< Thred printing stats
static Thread *writeT;           ///< Thread writing data to disk
static Thread *packT;            ///< Thread packing queued data in buffers
static Thread *senseT;           ///< Thread performing data sensig and analysis
static bool stopSensing=false;   ///< To stop the sensing process

static FILE *file;

static int statDroppedSamples=0; ///< Number of dropped sample due to fifo full, should be zero
static int statWriteFailed=0;    ///< Number of fwrite() that failed, should be zero
static int statWriteTime=0;      ///< Time to perform an fwrite() of a buffer
static int statMaxWriteTime=0;   ///< Max time to perform an fwrite() of a buffer
static int statQueuePush=0;      ///< Number of records successfully pushed in the queue
static int statBufferFilled=0;   ///< Number of buffers filled
static int statBufferWritten=0;  ///< Number of buffers successfully written to disk

/**
 * This thread performs sensor reading, data filtering and takes decisions from
 * the read data. It is the only high priority thread
 */
void senseThread(void *argv)
{
    try {
        auto wakeupTime=system_clock::now();
        for(;;)
        {
            if(stopSensing) break;
            Record sample=readSensors();
            processSensorData(sample);
            {
                FastInterruptDisableLock dLock;
                if(queuedSamples.IRQput(sample)==false) statDroppedSamples++;
                else statQueuePush++;
            }
            wakeupTime+=milliseconds(1);
            this_thread::sleep_until(wakeupTime);
        }
    } catch(exception& e) {
        printf("Error: senseThread failed due to an exception: %s\n",e.what());
    }
}

/**
 * This thread packs queued data in buffers to optimize write throughput
 */
void packThread(void *argv)
{
    try { 
        //Allocate buffers and put them in the empty list
        for(int i=0;i<numBuffers;i++)
            emptyList.push_back(new Record[recordPerBuffer]);
        
        for(;;)
        {
            //Get an empty buffer, wait if none is available
            Record *buffer;
            {
                Lock<FastMutex> l(listHandlingMutex);
                while(emptyList.empty())
                {
                    if(stopSensing) return;
                    listWaiting.wait(l);
                }
                buffer=emptyList.front();
                emptyList.pop_front();
            }
            
            //Read from the fifo and fill the buffer
            for(int i=0;i<recordPerBuffer;i++)
            {
                //FIXME: last partially filled buffer isn't written to disk
                if(stopSensing)
                {
                    delete[] buffer; //Don't forget to delete the buffer
                    return;
                }
                queuedSamples.get(buffer[i]);
            }
            statBufferFilled++;
            
            //Pass the buffer to the writeThread()
            {
                Lock<FastMutex> l(listHandlingMutex);
                fullList.push_back(buffer);
                listWaiting.broadcast();
            }
        }
    } catch(exception& e) {
        printf("Error: packThread failed due to an exception: %s\n",e.what());
    }
}

/**
 * This thread writes packed buffers to disk
 */
void writeThread(void *argv)
{
    try {
        for(;;)
        {
            //Get a full buffer, wait if none is available
            Record *buffer;
            {
                Lock<FastMutex> l(listHandlingMutex);
                while(fullList.empty())
                {
                    if(stopSensing) return;
                    listWaiting.wait(l);
                }
                buffer=fullList.front();
                fullList.pop_front();
            }
            
            //Write data to disk
            auto t=system_clock::now();
            ledOn();
            if(fwrite(buffer,1,bufferSize,file)!=bufferSize) statWriteFailed++;
            else statBufferWritten++;
            ledOff();
            auto d=system_clock::now()-t;
            statWriteTime=duration_cast<milliseconds>(d).count();
            statMaxWriteTime=max(statMaxWriteTime,statWriteTime);
            
            //Return the buffer to the packThread()
            {
                Lock<FastMutex> l(listHandlingMutex);
                emptyList.push_back(buffer);
                listWaiting.broadcast();
            }
        }
    } catch(exception& e) {
        printf("Error: writeThread failed due to an exception: %s\n",e.what());
    }
}

/**
 * This thread prints stats
 */
void statsThread(void *argv)
{
    try {
        for(;;)
        {
            if(stopSensing) return;
            this_thread::sleep_for(seconds(1));
            printf("ds:%d wf:%d wt:%d mwt:%d qp:%d bf:%d bw:%d\n",
                statDroppedSamples,statWriteFailed,statWriteTime,
                statMaxWriteTime,statQueuePush,statBufferFilled,statBufferWritten);
        }
    } catch(exception& e) {
        printf("Error: statsThread failed due to an exception: %s\n",e.what());
    }
}

/**
 * Start the sensing chain
 */
void startSensingChain()
{
    char filename[32];
    const int maxRetry=100;
    for(int i=0;i<maxRetry;i++)
    {
        sprintf(filename,"/sd/%02d.dat",i);
        struct stat st;
        if(stat(filename,&st)==0) continue; //File exists
        if(i==maxRetry-1)
            printf("Too many existing files, overwriting last\n");
        break;
    }
    file=fopen(filename,"w");
    if(file==NULL)
    {
        printf("Error opening log file\n");
        return;
    }
    setbuf(file,NULL);
    statsT=Thread::create(statsThread,4096,1,0,Thread::JOINABLE);
    writeT=Thread::create(writeThread,4096,1,0,Thread::JOINABLE);
    packT= Thread::create(packThread, 4096,1,0,Thread::JOINABLE);
    senseT=Thread::create(senseThread,4096,PRIORITY_MAX-1,0,Thread::JOINABLE);
    if(statsT==0 || writeT==0 || packT==0 || senseT==0)
        printf("Error: thread creation failure\n");
}

/**
 * Stop the sensing chain. Warning: this function may take
 * a long time (seconds) to complete
 */
void stopSensingChain()
{
    stopSensing=true;
    this_thread::sleep_for(milliseconds(20)); //Wait for threads to terminate
    //Wake threads eventually locked somewhere
    {
        FastInterruptDisableLock dLock;
        Record record;
        queuedSamples.IRQput(record);
    }
    {
        Lock<FastMutex> l(listHandlingMutex);
        listWaiting.broadcast();
    }
    statsT->join();
    writeT->join();
    packT->join();
    senseT->join();
    //Now threads are surely stopped
    while(emptyList.empty()==false)
    {
        delete[] emptyList.front();
        emptyList.pop_front();
    }
    while(fullList.empty()==false)
    {
        delete[] fullList.front();
        fullList.pop_front();
        //FIXME: don't just delete the buffer, write it to disk first
    }
    fclose(file);
}

int main()
{
    printf("Type enter to start\n");
    getchar();
    
    //Start threads
    startSensingChain();
    //Wait
    
    printf("Type enter to stop\n");
    getchar();
    
    //Stop threads and cleanup
    stopSensingChain();
    
    printf("Bye\n");
}
