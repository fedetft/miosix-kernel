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
        unsigned int period;//period in seconds
        uint32_t deviceId;
    };

    template <unsigned int N, unsigned int Q>
    class SmartSensing {
    public:

        SmartSensing(){
            status = (SmartSensingStatus*) getSmartSensingAreaBase();
            queue = (SSQueue<unsigned short, N>*)(getSmartSensingAreaBase() + sizeof (SmartSensingStatus));          
            signalOn = false;
        }

        static unsigned int getSmartSensingAreaBase() {
            return reinterpret_cast<unsigned int> (getBackupSramBase()) + 1020 * 4 - SmartSensing<N, Q>::getMemorySize() - sizeof (SmartSensingStatus);
        }

        int setQueue(uint32_t deviceId, unsigned int size, unsigned int period) {  
            Lock<Mutex> lock(sharedData);
            if ((size == 0) || (size > N)) {
                return -1;
            } else if (period < 1000) {
                return -2;
            }
            int index = getFirstFreeQueue();
            if (index < 0) {
                return -3;
            }                     
            initQueue((unsigned int) index, deviceId, size, period);            
            if(signalOn){
                newQueue.signal();
            }
            return 0;
        }

        int readQueue(int i, unsigned short* data, unsigned int size) {  
            Lock<Mutex> lock(sharedData);
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

        void onBoot() {
            if (firstBoot()) {
                init();
            } else {
                //debugInt(status->signature);
                //updateQueue(getTick());                
                IRQbootlog("In Coda:\r\n");
                debugInt(getTick());
                debugInt(getNextSecond(getTick(),status->nextSystemRestart*1000));
                debugInt(status->nextSystemRestart*1000);
                
                
                if (status->nextSystemRestart*1000 <= (unsigned long long)getTick()) {
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
                    IRQbootlog("GO To SLEEP!\r\n");
                    updateQueue(getTick()+500);//No problem
                    SuspendManager::suspend(getNextSecond(getTick(),status->nextSystemRestart*1000));
                }                
            }
        }

        void onSuspend(unsigned long long resumeTime) {  
            Lock<Mutex> lock(sharedData);
            status->nextSystemRestart = resumeTime;
            unsigned long long currentTime=getTick();
            updateQueue(currentTime+500);//No problem
            SuspendManager::suspend(getNextSecond(currentTime,status->nextSystemRestart*1000));                        
        }

        /**
         * Restituisce la memoria necessaria per le code (che andra sulla SRAM)
         * @return
         */
        static unsigned int getMemorySize() {
            return sizeof (SmartSensingStatus) + sizeof (SSQueue<unsigned short, N>) * Q;
        }
        
        //IF KON
        void startKernelDaemon(){
             Thread::create(daemon,768,Priority(),this);             
        }
        
    private:

        //PB & KON
        void updateQueue(unsigned long long time) {
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
                queue[i].size = 0;
                queue[i].remaining = 0;
            }
            status->nextSystemRestart = 0;
            status->signature=1337713;
        }
        
        //KON
        void initQueue(unsigned int i, uint32_t deviceId, unsigned int size, unsigned int period) {
            queue[i].deviceId = deviceId;
            queue[i].size = size;
            queue[i].remaining = size;
            queue[i].nextTime = getTick() + period;
            queue[i].period = period;
        }
        
        //IF KON        
        static void daemon(void* data){
            SmartSensing<N,Q>& ss=*((SmartSensing<N,Q>*)data);
            ss.signalOn=false;
            for(;;){
                ss.sharedData.lock();
//                
                ss.updateQueue(getTick());
                if(ss.getNextEvent(getTick(),0)==0){
                    ss.signalOn=true;
                    ss.newQueue.wait(ss.sharedData);
                    ss.signalOn=false;
                }
                long long nextRead = (long long)ss.getNextEvent(getTick(),0);
                long long currentTime = getTick();
                if(nextRead && (nextRead-currentTime>0)){
                    ss.sharedData.unlock();
                    Thread::sleep(nextRead-currentTime);        
                    ss.sharedData.lock();                                        
                }            
                ss.sharedData.unlock();
            }
                      
        }
        
        SmartSensingStatus* status;
        SSQueue<unsigned short, N>* queue;
        Mutex sharedData;
        bool signalOn;
        ConditionVariable newQueue; 
                
    };
    
    typedef SmartSensing<10,4> SMART_SENSING;
    SMART_SENSING& getSmartSensingDriver();

}
#endif	/* SMARTSENSING_H */

