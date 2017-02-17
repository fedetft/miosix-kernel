#include "vht.h"
#include "flopsync_vht.h"

using namespace std;
using namespace miosix;

static HRTB* hrtb=nullptr;
static Rtc* rtc=nullptr;

//Multiplicative factor VHT
double factor=1;
static unsigned int factorI=1;
static unsigned int factorD=0;
static unsigned int inverseFactorI=1;
static unsigned int inverseFactorD=0;

static long long fastNegMul(unsigned long long a,unsigned int bi, unsigned int bf){
    if(a<0){
        return -mul64x32d32(-a,bi,bf);
    }else{
        return mul64x32d32(a,bi,bf);
    }
}

long long VHT::getTick(){
    //printf("Theor: %lld,Factor : %.7f",HRTB::syncPointHrtTheoretical,factor);
    HRTB::aux1=hrtb->IRQgetCurrentTick()+HRTB::clockCorrection;
    
    long long tick;
    tick=HRTB::syncPointHrtTheoretical+mul64x32d32(HRTB::aux1-HRTB::syncPointHrtSlave,factorI,factorD);
    return tick;
}

long long VHT::getOriginalTick(long long tick){
    return mul64x32d32((tick-HRTB::syncPointHrtTheoretical),inverseFactorI,inverseFactorD)+HRTB::syncPointHrtSlave;
}

long long VHT::correctTickWithCurrentWindow(long long tick){
    return HRTB::syncPointHrtTheoretical+fastNegMul(tick-HRTB::syncPointHrtSlave,factorI,factorD);
}

void VHT::stopResyncSoft(){
    softEnable=false;
}

void VHT::startResyncSoft(){
    softEnable=true;
}

VHT& VHT::instance(){
    static VHT vht;
    return vht;
}

VHT::VHT() {
    hrtb=&HRTB::instance();
    rtc=&Rtc::instance();
    
    // Thread that is waken up by the timer2 to perform the clock correction
    HRTB::flopsyncThread=Thread::create(&VHT::doRun,2048,1,this);
    TIMER2->IEN |= TIMER_IEN_CC2;
}

void VHT::doRun(void* arg)
{
    reinterpret_cast<VHT*>(arg)->loop();
}

void VHT::loop() {
    initDebugPins();
    long long hrtT;
    long long rtcT;
    FlopsyncVHT f;
    
    int tempPendingVhtSync;				    ///< Number of sync acquired in a round
    while(1){
        Thread::wait();
        {
            //Read in atomic context the 2 SHARED values to run flopsync
            FastInterruptDisableLock dLock;
            //rtcT=HRTB::nextSyncPointRtc-HRTB::syncPeriodRtc;
            hrtT=HRTB::syncPointHrtMaster;
            tempPendingVhtSync=VHT::pendingVhtSync;
            VHT::pendingVhtSync=0;
        }

        HRTB::syncPointHrtSlave += (HRTB::syncPeriodHrt + HRTB::clockCorrectionFlopsync)*tempPendingVhtSync;
        //Master è quello timestampato correttamente, il nostro punto di riferimento
        HRTB::error = hrtT - (HRTB::syncPointHrtSlave);
        int u=f.computeCorrection(HRTB::error);
        

        
        if(VHT::softEnable)
        {
            PauseKernelLock pkLock;
            // Single instruction that update the error variable, 
            // interrupt can occur, but not thread preemption
            HRTB::clockCorrectionFlopsync=u;

            //Simple calculation of factor, very inefficient
            factor = (double)HRTB::syncPeriodHrt/(HRTB::syncPeriodHrt+HRTB::clockCorrectionFlopsync);
            //efficient way to calculate the factor T/(T+u(k))
            long long temp=(HRTB::syncPeriodHrt<<32)/(HRTB::syncPeriodHrt+HRTB::clockCorrectionFlopsync)-4294967296;
            factorI = temp>0 ? 1:0;
            factorD = (unsigned int) temp;
            //calculate inverse of previous factor (T+u(k))/T
            temp = ((HRTB::syncPeriodHrt+HRTB::clockCorrectionFlopsync)<<32)/HRTB::syncPeriodHrt-4294967296;
            inverseFactorI = temp>0 ? 1:0;
            inverseFactorD = (unsigned int)temp;
        } 
//      printf("%lld\n",HRTB::clockCorrectionFlopsync);
        //This printf shouldn't be in here because is very slow 
        printf( "HRT bare:%lld, RTC %lld, next:%lld, COMP1:%lu basicCorr:%lld\n\t"
                "Theor:%lld, Master:%lld, Slave:%lld\n\t"
                "Error:%lld, FSync corr:%lld, PendingSync:%d\n\n",
            hrtb->IRQgetCurrentTick(),
            rtc->getValue(),
            HRTB::nextSyncPointRtc,
            RTC->COMP1,
            HRTB::clockCorrection,
            HRTB::syncPointHrtTheoretical,
            HRTB::syncPointHrtMaster,
            HRTB::syncPointHrtSlave,
            HRTB::error,
            HRTB::clockCorrectionFlopsync,
            tempPendingVhtSync);
        
         
        // If the event is enough in the future, correct the CStimer. NO!
        // Fact: this is a thread, so the scheduler set an a periodic interrupt until 
        //      all the threads go to sleep/wait. 
        // So, if there was a long wait/sleep already set in the register, 
        // this value is continuously overridden by this thread call. 
        // The VHT correction is done in the IRQsetInterrupt, 
        // that use always fresh window to correct the given time
        
        // Radio timer and GPIO timer should be corrected?
        // In first approximation yes, but usually I want to trigger after few times (few ms)
        // Doing some calculation, we found out that in the worst case of skew,
        // after 13ms we have 1tick of error. So we in this case we can disable 
        // the sync VHT but be sure that we won't lose precision. 
        // If you want trigger after a long time, let's say a year, 
        // you can call thread::sleep for that time and then call the trigger function
    }
}

int VHT::pendingVhtSync=0;
bool VHT::softEnable=true;
bool VHT::hardEnable=true;




