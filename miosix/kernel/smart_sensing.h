/*
 * File:   SmartSensing.h
 * Author: Alessandro Rizzi
 *
 * Created on 18 novembre 2012, 12.48
 */

#ifndef SMARTSENSING_H
#define	SMARTSENSING_H

#include "logging.h"
#include "kernel.h"
#include "interfaces/suspend_support.h"
#include "suspend_manager.h"
#include "interfaces/adc_driver.h"

#define CANARY 0x55AA55AA
#define CHECK_CANARY

namespace miosix {    
    
    void debugInt(unsigned int i);    

    struct SmartSensingStatus {
#ifdef CHECK_CANARY
        /**
         * @brief canary
         * Canary to check if the backup ram area of the class has been corrupted
         */
        unsigned long int canary;
#endif
        /**
         * @brief nextSystemRestart
         * Time in seconds of the next system restart
         */
        unsigned long long nextSystemRestart;
    };


    template <typename T, unsigned int N>
    /**
     * @brief The SSQueue struct
     * Structure which holds a queue
     */
    struct SSQueue {
        T data[N];
        unsigned int size;
        unsigned int remaining;
        /**
         * @brief nextTime next time in milliseconds
         */
        unsigned long long int nextTime;
        /**
         * @brief period period in milliseconds
         */
        unsigned int period;
        uint32_t deviceId;
        pid_t processId;
    };

    template <unsigned int N, unsigned int Q>
    class SmartSensing {

        /**
         * @brief SmartSensing
         *  Constructor of the class which initialize the temporary variables (the ones which don't persist)
         *  It's private in order to prevent instantiations from the outside
         */
        SmartSensing(){
            status = (SmartSensingStatus*) getSmartSensingAreaBase();
            queue = (SSQueue<unsigned short, N>*)(getSmartSensingAreaBase() + sizeof (SmartSensingStatus));                      
            for(unsigned int i=0;i<Q;i++){
                threadId[i]=NULL;//RESET THE THREADID
            }
            smartSensingThread=NULL;
        }
    public:

        /**
         * @brief getSmartSensingAreaBase
         * Retrive the start address of the queue structure in backup ram
         * @return location in backup ram of the structure used by the class
         */
        static unsigned int getSmartSensingAreaBase() {
            return reinterpret_cast<unsigned int> (getBackupSramBase()) + 1020 * 4 - SmartSensing<N, Q>::getMemorySize() - sizeof (SmartSensingStatus);
        }

        /**
         * @brief setQueue
         * Initialize a new queue
         * @param processId id of the process which own the queue
         * @param threadId id of the thread which invoke the method
         * @param deviceId id of the selected source
         * @param size number of sample to be captured
         * @param sampling interval in millisecond
         * @return 0 if the work is scheduled, a negative integer if an error occourred
         */
        int setQueue(pid_t processId,Thread* threadId,uint32_t deviceId, unsigned int size, unsigned int period) {
            Lock<Mutex> lock(sharedData);            

            if ((size == 0) || (size > N)) {
                return -1;
            } else if (period < 1000) {
                return -2;
            } else if (getQueueFromProcessId(processId)!=-1) {//Check that no other queues exist fo the process
                return -3;
            }

            int index = getFirstFreeQueue();
            if (index < 0) {
                return -3;
            }                     
            initQueue((unsigned int) index, processId,threadId, deviceId, size, period);
            if(smartSensingThread!=NULL){
                smartSensingThread->wakeup();
                smartSensingThread=NULL;
            }
            return 0;
        }

        /**
         * @brief readQueue
         * Retrive data from a completed read
         * @param processId id of the process which own the queue
         * @param data pointer to the place where the reads will be stored
         * @return -1 if an error occourred, number of sample written otherwise
         *  the error can occour if the process hasn't a queue or if the queue
         *  hasn't been completely filled yet.
         *  In every case the process queue is resetted.
         */
        int readQueue(pid_t processId, unsigned short* data) {
            Lock<Mutex> lock(sharedData);
            int i=getQueueFromProcessId(processId);
            if(i<0){
                return -1;
            }
            if(queue[i].remaining!=0){
                resetQueue(i);
                return -1;
            }
            unsigned int writingSize = queue[i].size;
            for (unsigned int j = 0; j < writingSize; j++) {
                data[j] = queue[i].data[j];
            }
            resetQueue(i);
            return writingSize;
        }

        /**
         * @brief onBoot
         * Hook of the boot process
         */
        void onBoot() {
            if (firstBoot()) {
                init();
            } else {
#ifdef CHECK_CANARY
                if(status->canary!=CANARY){
                    errorHandler(UNEXPECTED);
                }
#endif
                //debugInt(status->signature);
                updateQueue(getTick());
                IRQbootlog("In Coda:\r\n");
                debugInt(getTick());
                debugInt(getNextSecond(getTick(),status->nextSystemRestart*1000));
                debugInt(status->nextSystemRestart*1000);

                if ((completedTask)||(status->nextSystemRestart*1000 <= (unsigned long long)getTick())) {
                    status->nextSystemRestart=0;
                    IRQbootlog("SS: Restart\r\n");
                    return;
                }
                else if (getNextEvent(getTick(),status->nextSystemRestart*1000) == 0) {
                    errorHandler(UNEXPECTED);
                }                
                else {                    
                    updateQueue(getTick()+500);//No problem
                    if(completedTask){
                        return;
                    }
                    IRQbootlog("GO To SLEEP!\r\n");
                    SuspendManager::suspend(getNextSecond(getTick(),status->nextSystemRestart*1000));
                }                
            }
        }

        /**
         * @brief onSuspend
         * Hook of the suspension process
         * @param resumeTime time in seconds to be suspended
         */
        void onSuspend(unsigned long long resumeTime) {  
            Lock<Mutex> lock(sharedData);
            status->nextSystemRestart = resumeTime;
            unsigned long long currentTime=getTick();
            updateQueue(currentTime+500);//No problem
            if(completedTask){
                return;
            }
            SuspendManager::suspend(getNextSecond(currentTime,status->nextSystemRestart*1000));                        
        }

        /**
         * @brief getMemorySize
         * Calculate the required memory to permit hte hibernation of the class
         * @return size in bytes of the required backup SRAM memory
         */
        static unsigned int getMemorySize() {
            return sizeof (SmartSensingStatus) + sizeof (SSQueue<unsigned short, N>) * Q;
        }
        
        /**
         * @brief startKernelDaemon
         * Starts the deamon which perform reads when the kernel is active
         */
        static void startKernelDaemon(){
             getSmartSensingInstance()->wakeCompletedProcess();
             Thread::create(daemonThread,1536);
        }

        /**
         * @brief getSmartSensingInstance
         * Return a pointer to the instance of the class
         * @return poiter to the instance of the class
         */
        static SmartSensing<N,Q>* getSmartSensingInstance(){
            return &smartSensingInstance;
        }

        /**
         * @brief cleanUp
         * Removes all the queues which belongs to the given process
         * @param processId id of the process
         */
        void cleanUp(pid_t processId){
            Lock<Mutex> lock(sharedData);
            while(resetQueue(getQueueFromProcessId(processId)));
        }
        
    private:

        /**
         * @brief updateQueue
         * Update all the initialized queues
         * Can be invoked either during boot phase or when the kernel is active
         * @param time time in milliseconds. All the reads which are before it
         * will be performed.
         */
        void updateQueue(unsigned long long time) {
            completedTask=false;
            //IRQbootlog("Updated!\r\n");            
            for (unsigned int i = 0; i < Q; i++) {
                if ((queue[i].remaining > 0) && (queue[i].nextTime <= time)) {                    
                    readQueue(i);
                    debugInt(queue[i].nextTime);                    
                    queue[i].nextTime += queue[i].period;
                    if(queue[i].nextTime <= time){
                       // IRQbootlog("Emergency!\r\n");
                        queue[i].nextTime = time + queue[i].period;
                    }
                    if(queue[i].remaining==0){
                        completedTask=true;
                    }
                    //debugInt(time);
                    //debugInt(queue[i].nextTime);                    
                    //IRQbootlog("R&I!\r\n");
                }
            }
        }
        
        /**
         * @brief getNextSecond
         * Retrive next time in seconds in which the next event will happen
         * Can be invoked either during boot phase or when the kernel is active
         * @param currentTime time in millisecond to be considered as current time
         * @param minTime if different from 0 it counts as an additional event time in milliseconds
         * @return next event time in seconds
         */
        unsigned int getNextSecond(unsigned long long currentTime, unsigned long long minTime) const{
            unsigned int nextEvent=getNextEvent(currentTime,minTime);
            if((currentTime%1000)>(nextEvent%1000)){
                return nextEvent/1000;
            }
            return (nextEvent+999)/1000;
        }

        /**
         * @brief getNextEvent
         * Retrive next time in milliseconds in which the next event will happen
         * Can be invoked either during boot phase or when the kernel is active
         * @param currentTime time in millisecond to be considered as current time
         * @param minTime if different from 0 it counts as an additional event time in milliseconds
         * @return next event time in milliseconds
         */
        unsigned long long getNextEvent(unsigned long long currentTime,unsigned long long minTime) const{                                  
            for (unsigned int i = 0; i < Q; i++) {
                if ((queue[i].size > 0) && (queue[i].remaining > 0) && (queue[i].nextTime>currentTime)) {
                    if ((minTime == 0) || ((minTime > queue[i].nextTime))) {                        
                        minTime = queue[i].nextTime;
                    }
                }
            }
            return minTime;
        }
        
        /**
         * @brief getFirstFreeQueue
         * Retrive the first queue available
         * It's supposed to be invoked only if the kernel is active
         * @return index of the first queue, -1 if none available
         */
        int getFirstFreeQueue() const{
            for (unsigned int i = 0; i < Q; i++) {
                if (queue[i].size == 0) {
                    return i;
                }
            }
            return -1;
        }
        
        /**
         * @brief readQueue
         * Performs a read on the selected queue
         * Can be invoked either during boot phase or when the kernel is active
         * No checks are performed either on the index or if the queue is full
         * @param i index of the selected queue
         */
        void readQueue(int i) {
            queue[i].data[queue[i].size - queue[i].remaining] = AdcDriver::read(queue[i].deviceId);
            queue[i].remaining--;
        }
        
        /**
         * @brief init
         * Initialize the persistent variables of the class
         * Must be called only on the first boot
         */
        void init() {
            for (unsigned int i = 0; i < Q; i++) {
                resetQueue(i);
            }
            status->nextSystemRestart = 0;
#ifdef CHECK_CANARY
            status->canary=CANARY;
#endif
        }

        /**
         * @brief wakeCompletedProcess
         * Wake up all the processes whose tasks have been terminated
         * Can be invoked only when the kernel is active
         */
        void wakeCompletedProcess(){
            for(unsigned int i=0;i<Q;i++){
                if((queue[i].size>0)&&(queue[i].remaining==0)){
                    if(threadId[i]!=NULL){
                        threadId[i]->wakeup();
                    }
                    else{
                        SuspendManager::wakeUpProcess(queue[i].processId);
                    }
                }
            }
        }

        /**
         * @brief resetQueue
         * Set a given queue as available
         * Can be invoked either during boot phase or when the kernel is active
         * @param i index of the queue
         * @return false if the given index is invalid, true otherwise
         */
        bool resetQueue(unsigned int i){
            if((i<0)||(i>=Q)){
                return false;
            }
            queue[i].size = 0;
            queue[i].remaining = 0;
            return true;
        }
        
        /**
         * @brief initQueue
         * Initialize a queue
         * It's supposed to be invoked only if the kernel is active
         * @param i index of a free queue
         * @param processId id of the process which owns the corresponding task
         * @param threadId id of the thread which owns the corresponding task
         * @param deviceId id which carries the information on the input source of the reads
         * @param size number of reads to be performed
         * @param period time in milliseconds between two reads
         */
        void initQueue(unsigned int i, pid_t processId, Thread* threadId, uint32_t deviceId, unsigned int size, unsigned int period) {
            queue[i].deviceId = deviceId;
            queue[i].size = size;
            queue[i].remaining = size;
            queue[i].nextTime = getTick() + period;
            queue[i].period = period;
            queue[i].processId=processId;
            this->threadId[i]=threadId;
        }

        /**
         * @brief getQueueFromProcessId
         * Retrive the queue associated to a given process
         * Can be invoked either during boot phase or when the kernel is active
         * @param processId id of the process which owns the queue
         * @return index of the requested queue, -1 if no queue correspond the the given process
         */
        int getQueueFromProcessId(pid_t processId) const{
            for(unsigned int i=0;i<Q;i++){
                if ((queue[i].size>0) && (queue[i].processId==processId)) {
                    return (int)i;
                }
            }
            return -1;
        }

        /**
         * @brief getDaemonSleepTime
         * Retrive the time the kernel daemon must wait before performing a read
         * It's supposed to be invoked only if the kernel is active
         * @return time in milliseconds before the next read
         */
        unsigned int getDaemonSleepTime() const{
            long long nextRead = (long long)getNextEvent(getTick(),0);
            long long currentTime = getTick();
            if(nextRead && (nextRead-currentTime>0)){
                return nextRead-currentTime;
            }
            return 0;
        }

        /**
         * @brief innerThread
         * This method implements the kernel daemon loop
         * It's supposed to be invoked only if the kernel is active
         */
        void innerThread(){
            Lock<Mutex> lock(sharedData);
            for(;;){
                updateQueue(getTick());
                wakeCompletedProcess();
                if(getNextEvent(getTick(),0)==0){
                    smartSensingThread=Thread::getCurrentThread();
                    {
                        Unlock<Mutex> unlock(sharedData);
                        Thread::wait();
                    }
                }
                unsigned int sleepTime=getDaemonSleepTime();
                if(sleepTime>0){
                        Unlock<Mutex> unlock(sharedData);
                        Thread::sleep(sleepTime);
                }
            }

        }
        
        /**
         * @brief daemonThread
         * Entry point of the kernel daemon
         * It's supposed to be invoked only if the kernel is active
         * @param data unused
         */
        static void daemonThread(void* data){
            getSmartSensingInstance()->innerThread();
        }

        /**
         * @brief status
         * Additional persistent information: a canary and next system restart time
         */
        SmartSensingStatus* status;

        /**
         * @brief queue
         * Persistent information of the queues
         */
        SSQueue<unsigned short, N>* queue;

        /**
         * @brief threadId
         * Temporary information of the thread id associated to each queue
         */
        Thread* threadId[Q];

        /**
         * @brief smartSensingThread
         * Thread of the kernel daemon
         */
        Thread* smartSensingThread;

        /**
         * @brief sharedData
         * Mutex which controls the access to the class data
         */
        Mutex sharedData;

        /**
         * @brief completedTask
         * True if there are some processes which are waiting to be woken up
         * Useful to decide if continue the kernel boot or suspend again
         */
        bool completedTask;

        /**
         * @brief smartSensingInstance
         * Singleton instance of the class
         */
        static SmartSensing smartSensingInstance;

    };    

    template <unsigned int N,unsigned int Q>
    SmartSensing<N,Q> SmartSensing<N,Q>::smartSensingInstance;

    typedef SmartSensing<10,4> SMART_SENSING;


}
#endif	/* SMARTSENSING_H */

