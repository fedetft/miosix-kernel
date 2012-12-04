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

namespace miosix {    
    
    void debugInt(unsigned int i);    

    struct SmartSensingStatus {
        unsigned long int signature;
        unsigned long long nextSystemRestart;
    };

    template <typename T, unsigned int N>
    struct SSQueue {
        T data[N];
        unsigned int size;
        unsigned int remaining;
        unsigned long long int nextTime;//nextTime in milliseconds
        unsigned int period;//period in milliseconds
        uint32_t deviceId;
        pid_t processId;
    };

    template <unsigned int N, unsigned int Q>
    class SmartSensing {

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
         * Retrive the start address of the queue structure in backup ram
         * @brief getSmartSensingAreaBase
         * @return location in backup ram of the structure used by the class
         */
        static unsigned int getSmartSensingAreaBase() {
            return reinterpret_cast<unsigned int> (getBackupSramBase()) + 1020 * 4 - SmartSensing<N, Q>::getMemorySize() - sizeof (SmartSensingStatus);
        }

        /**
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
         * Retrive data from a completed read
         * @param processId id of the process which own the queue
         * @param data pointer to the place where the reads will be stored
         * @param size number of samples that can be saved
         * @return -1 if an error occourred, number of sample written otherwise
         */
        int readQueue(pid_t processId, unsigned short* data, unsigned int size) {
            Lock<Mutex> lock(sharedData);
            int i=getQueueFromProcessId(processId);
            if(i<0){
                return -1;
            }
            unsigned int availableData = queue[i].size - queue[i].remaining;
            unsigned int writingSize = std::min(size, availableData);
            for (unsigned int j = 0; j < writingSize; j++) {
                data[j] = queue[i].data[j];                
            }
            //queue[i].remaining += writingSize;
            queue[i].size-=writingSize;            
            if (writingSize == availableData) {//CHECK                
                return writingSize;
            }
            for (unsigned int j = 0; j < size - writingSize; j++) {
                queue[i].data[j] = queue[i].data[j + writingSize];
            }            
            return writingSize;
        }

        /**
         * Hook of the boot process
         */
        void onBoot() {
            if (firstBoot()) {
                init();
            } else {
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
                    IRQbootlog("SS: What???\r\n");
                    //SuspendManager::suspend(status->nextSystemRestart);
                    return;
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
         * Restituisce la memoria necessaria per le code (che andra sulla SRAM)
         * @return
         */
        static unsigned int getMemorySize() {
            return sizeof (SmartSensingStatus) + sizeof (SSQueue<unsigned short, N>) * Q;
        }
        
        /**
         * Starts the deamon which perform reads when the kernel is active
         */
        //IF KON
        static void startKernelDaemon(){
             getSmartSensingInstance()->wakeCompletedProcess();
             Thread::create(daemonThread,1536);
        }

        /**
         * Return a pointer to the instance of the class
         * @return poiter to the instance of the class
         */
        static SmartSensing<N,Q>* getSmartSensingInstance(){
            return &smartSensingInstance;
        }

        /**
         * Removes all the queues which belongs to the given process
         * @param processId id of the process
         */
        //IF KON
        void cleanUp(pid_t processId){
            Lock<Mutex> lock(sharedData);
            while(resetQueue(getQueueFromProcessId(processId)));
        }
        
    private:

        //PB & KON
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
        
        //PB & KON
        unsigned int getNextSecond(unsigned long long currentTime, unsigned long long minTime) const{
            unsigned int nextEvent=getNextEvent(currentTime,minTime);
            if((currentTime%1000)>(nextEvent%1000)){
                return nextEvent/1000;
            }
            return (nextEvent+999)/1000;
        }

        //PB & KON
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
        
        //KON
        int getFirstFreeQueue() const{
            for (unsigned int i = 0; i < Q; i++) {
                if (queue[i].size == 0) {
                    return i;
                }
            }
            return -1;
        }
        
        //PB & KON
        void readQueue(int i) {
            //TODO Check
            //remaining>0
            queue[i].data[queue[i].size - queue[i].remaining] = AdcDriver::read(queue[i].deviceId);
            queue[i].remaining--;
        }
        
        //PB
        void init() {
            for (unsigned int i = 0; i < Q; i++) {
                resetQueue(i);
            }
            status->nextSystemRestart = 0;
            status->signature=1337713;
        }

        //KON
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

        //PB & KON
        bool resetQueue(unsigned int i){
            if((i<0)||(i>=Q)){
                return false;
            }
            queue[i].size = 0;
            queue[i].remaining = 0;
            return true;
        }
        
        //KON
        void initQueue(unsigned int i, pid_t processId, Thread* threadId, uint32_t deviceId, unsigned int size, unsigned int period) {
            queue[i].deviceId = deviceId;
            queue[i].size = size;
            queue[i].remaining = size;
            queue[i].nextTime = getTick() + period;
            queue[i].period = period;
            queue[i].processId=processId;
            this->threadId[i]=threadId;
        }

        //PB & KON
        int getQueueFromProcessId(pid_t processId) const{
            for(unsigned int i=0;i<Q;i++){
                if ((queue[i].size>0) && (queue[i].processId==processId)) {
                    return (int)i;
                }
            }
            return -1;
        }

        //KON
        unsigned int getDaemonSleepTime() const{
            long long nextRead = (long long)getNextEvent(getTick(),0);
            long long currentTime = getTick();
            if(nextRead && (nextRead-currentTime>0)){
                return nextRead-currentTime;
            }
            return 0;
        }

        //KON
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
        
        //IF KON        
        static void daemonThread(void* data){
            getSmartSensingInstance()->innerThread();
        }
        
        SmartSensingStatus* status;

        SSQueue<unsigned short, N>* queue;

        Thread* threadId[Q];

        Thread* smartSensingThread;

        Mutex sharedData;

        bool completedTask;

        static SmartSensing smartSensingInstance;

    };    

    template <unsigned int N,unsigned int Q>
    SmartSensing<N,Q> SmartSensing<N,Q>::smartSensingInstance;

    typedef SmartSensing<10,4> SMART_SENSING;


}
#endif	/* SMARTSENSING_H */

