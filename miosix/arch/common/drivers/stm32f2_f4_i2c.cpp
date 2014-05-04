
#include "stm32f2_f4_i2c.h"
#include <miosix.h>
#include <kernel/scheduler/scheduler.h>

using namespace miosix;

static bool error;        ///< Set to true by IRQ on error
static Thread *waiting=0; ///< Thread waiting for an operation to complete

/**
 * DMA I2C rx end of transfer
 */
void __attribute__((naked)) DMA1_Stream0_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z20I2C1rxDmaHandlerImplv");
    restoreContext();
}

/**
 * DMA I2C tx end of transfer actual implementation
 */
void __attribute__((used)) I2C1rxDmaHandlerImpl()
{
    DMA1->LIFCR=DMA_LIFCR_CTCIF0
              | DMA_LIFCR_CTEIF0
              | DMA_LIFCR_CDMEIF0
              | DMA_LIFCR_CFEIF0;
    I2C1->CR1 |= I2C_CR1_STOP;
    if(waiting==0) return;
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=0;
}

/**
 * DMA I2C tx end of transfer
 */
void DMA1_Stream7_IRQHandler()
{
    DMA1->HIFCR=DMA_HIFCR_CTCIF7
              | DMA_HIFCR_CTEIF7
              | DMA_HIFCR_CDMEIF7
              | DMA_HIFCR_CFEIF7;
    
    //We can't just wake the thread because the I2C is double buffered, and this
    //interrupt is fired at the same time as the second last byte is starting
    //to be sent out of the bus. If we return now, the main code would send a
    //stop condiotion too soon, and the last byte would never be sent. Instead,
    //we change from DMA mode to IRQ mode, so when the second last byte is sent,
    //that interrupt is fired and the last byte is sent out.
    //Note that since no thread is awakened from this IRQ, there's no need for
    //the saveContext(), restoreContext() and __attribute__((naked))
    I2C1->CR2 &= ~I2C_CR2_DMAEN;
    I2C1->CR2 |= I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN;
}

/**
 * I2C address sent interrupt
 */
void __attribute__((naked)) I2C1_EV_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z15I2C1HandlerImplv");
    restoreContext();
}

/**
 * I2C address sent interrupt actual implementation
 */
void __attribute__((used)) I2C1HandlerImpl()
{
    //When called to resolve the last byte not sent issue, clearing
    //I2C_CR2_ITBUFEN prevents this interrupt being re-entered forever, as
    //it does not send another byte to the I2C, so the interrupt would remain
    //pending. When called after the start bit has been sent, clearing
    //I2C_CR2_ITEVTEN prevents the same infinite re-enter as this interrupt
    //does not start an address transmission, which is necessary to stop
    //this interrupt from being pending
    I2C1->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN);
    if(waiting==0) return;
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=0;
}

/**
 * I2C error interrupt
 */
void __attribute__((naked)) I2C1_ER_IRQHandler()
{
    saveContext();
    asm volatile("bl _Z18I2C1errHandlerImplv");
    restoreContext();
}

/**
 * I2C error interrupt actual implementation
 */
void __attribute__((used)) I2C1errHandlerImpl()
{
    I2C1->SR1=0; //Clear error flags
    error=true;
    if(waiting==0) return;
    waiting->IRQwakeup();
    if(waiting->IRQgetPriority()>Thread::IRQgetCurrentThread()->IRQgetPriority())
        Scheduler::IRQfindNextThread();
    waiting=0;
}

namespace miosix {

//
// class I2C
//

I2C1Driver& I2C1Driver::instance()
{
    static I2C1Driver singleton;
    return singleton;
}

void I2C1Driver::init()
{
    //I2C devices are connected to APB1, whose frequency is the system clock
    //divided by a value set in the PPRE1 bits of RCC->CFGR
    const int ppre1=(RCC->CFGR & RCC_CFGR_PPRE1)>>10;
    const int divFactor= (ppre1 & 1<<2) ? (2<<(ppre1 & 0x3)) : 1;
    const int fpclk1=SystemCoreClock/divFactor;
    //iprintf("fpclk1=%d\n",fpclk1);
    
    {
        FastInterruptDisableLock dLock;
        RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
        RCC_SYNC();
        RCC->APB1ENR |= RCC_APB1ENR_I2C1EN; //Enable clock gating
        RCC_SYNC();
    }
    
    NVIC_SetPriority(DMA1_Stream7_IRQn,10);//Low priority for DMA
    NVIC_ClearPendingIRQ(DMA1_Stream7_IRQn);//DMA1 stream 7 channel 1 = I2C1 TX 
    NVIC_EnableIRQ(DMA1_Stream7_IRQn);
    
    NVIC_SetPriority(DMA1_Stream0_IRQn,10);//Low priority for DMA
    NVIC_ClearPendingIRQ(DMA1_Stream0_IRQn);//DMA1 stream 0 channel 1 = I2C1 RX 
    NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    
    NVIC_SetPriority(I2C1_EV_IRQn,10);//Low priority for I2C
    NVIC_ClearPendingIRQ(I2C1_EV_IRQn);
    NVIC_EnableIRQ(I2C1_EV_IRQn);
    
    NVIC_SetPriority(I2C1_ER_IRQn,10);
    NVIC_ClearPendingIRQ(I2C1_ER_IRQn);
    NVIC_EnableIRQ(I2C1_ER_IRQn);
    
    I2C1->CR1=I2C_CR1_SWRST;
    I2C1->CR1=0;
    I2C1->CR2=fpclk1/1000000; //Set pclk frequency in MHz
    //This sets the duration of both Thigh and Tlow (master mode))
    const int spiSpeed=100000; //100KHz
    I2C1->CCR=std::max(4,fpclk1/(2*spiSpeed)); //Duty=2, standard mode (100KHz)
    //Datasheet says with I2C @ 100KHz, maximum SCL rise time is 1000ns
    //Need to change formula if I2C needs to run @ 400kHz
    I2C1->TRISE=fpclk1/1000000+1;
    I2C1->CR1=I2C_CR1_PE; //Enable peripheral
}

bool I2C1Driver::send(unsigned char address, const void *data, int len)
{
    address &= 0xfe; //Mask bit 0, as we are writing
    if(start(address)==false || (I2C1->SR2 & I2C_SR2_TRA)==0)
    {
        I2C1->CR1 |= I2C_CR1_STOP;
        return false;
    }

    error=false;
    waiting=Thread::getCurrentThread();
    DMA1_Stream7->CR=0;
    DMA1_Stream7->PAR=reinterpret_cast<unsigned int>(&I2C1->DR);
    DMA1_Stream7->M0AR=reinterpret_cast<unsigned int>(data);
    DMA1_Stream7->NDTR=len;
    DMA1_Stream7->FCR=DMA_SxFCR_FEIE
                    | DMA_SxFCR_DMDIS;
    DMA1_Stream7->CR=DMA_SxCR_CHSEL_0 //Channel 1
                   | DMA_SxCR_MINC    //Increment memory pointer
                   | DMA_SxCR_DIR_0   //Memory to peripheral
                   | DMA_SxCR_TCIE    //Interrupt on transfer complete
                   | DMA_SxCR_TEIE    //Interrupt on transfer error
                   | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                   | DMA_SxCR_EN;     //Start DMA
    
    //Enable DMA in the I2C peripheral *after* having configured the DMA
    //peripheral, or a spurious interrupt is triggered
    I2C1->CR2 |= I2C_CR2_DMAEN | I2C_CR2_ITERREN;
 
    {
        FastInterruptDisableLock dLock;
        while(waiting)
        {
            waiting->IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    
    DMA1_Stream7->CR=0;
    
    //The DMA interrupt routine changes the interrupt flags!
    I2C1->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    
    /*
     * The main idea of this driver is to avoid having the processor spinning
     * waiting on some status flag. Why? Because I2C is slow compared to a
     * modern processor. A 120MHz core does 1200 clock cycles in the time it
     * takes to transfer a single bit through an I2C clocked at 100KHz.
     * This time could be better spent doing a context switch and letting
     * another thread do useful work, or (and Miosix does it automatically if
     * there are no ready threads) sleeping the processor core. However,
     * I'm quite disappointed by the STM32 I2C peripheral, as it seems overly
     * complicated to use. To come close to achieving this goal I had to
     * orchestrate among *four* interrupt handlers, two of the DMA, and two
     * of the I2C itself. And in the end, what's even more disappointing, is
     * that I haven't found a way to completely avoid spinning. Why?
     * There's no interrupt that's fired when the stop bit is sent!
     * And what's worse, the documentation says that after you set the stop
     * bit in the CR2 register you can't write to it again (for example, to send
     * a start bit because two i2c api calls are made back to back) until the
     * MSL bit is cleared. But there's no interrupt tied to that event!
     * What's worse, is that the closest interrupt flag I've found when doing
     * an I2C send is fired when the last byte is *beginning* to be sent.
     * Maybe I haven't searched well enough, but the fact is I found nothing,
     * so this code below spins for 8 data bits of the last byte plus the ack
     * bit, plus the stop bit. That's 12000 wasted CPU cycles. Thanks, ST...
     */
    I2C1->CR1 |= I2C_CR1_STOP;
    while(I2C1->SR2 & I2C_SR2_MSL) ; //Wait for stop bit sent
    return !error;
}

bool I2C1Driver::recv(unsigned char address, void *data, int len)
{
    address |= 0x01;
    if(start(address,len==1)==false || I2C1->SR2 & I2C_SR2_TRA)
    {
        I2C1->CR1 |= I2C_CR1_STOP;
        return false;
    }

    I2C1->CR2 |= I2C_CR2_DMAEN | I2C_CR2_LAST | I2C_CR2_ITERREN;
    
    error=false;
    waiting=Thread::getCurrentThread();
    DMA1_Stream0->CR=0;
    DMA1_Stream0->PAR=reinterpret_cast<unsigned int>(&I2C1->DR);
    DMA1_Stream0->M0AR=reinterpret_cast<unsigned int>(data);
    DMA1_Stream0->NDTR=len;
    DMA1_Stream0->FCR=DMA_SxFCR_FEIE
                    | DMA_SxFCR_DMDIS;
    DMA1_Stream0->CR=DMA_SxCR_CHSEL_0 //Channel 1
                   | DMA_SxCR_MINC    //Increment memory pointer
                   | DMA_SxCR_TCIE    //Interrupt on transfer complete
                   | DMA_SxCR_TEIE    //Interrupt on transfer error
                   | DMA_SxCR_DMEIE   //Interrupt on direct mode error
                   | DMA_SxCR_EN;     //Start DMA

    {
        FastInterruptDisableLock dLock;
        while(waiting)
        {
            waiting->IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }

    DMA1_Stream7->CR=0;
    
    I2C1->CR2 &= ~(I2C_CR2_DMAEN | I2C_CR2_LAST | I2C_CR2_ITERREN);
    while(I2C1->SR2 & I2C_SR2_MSL) ; //Wait for stop bit sent
    return !error;
}

bool I2C1Driver::start(unsigned char address, bool immediateNak)
{
    I2C1->CR1 |= I2C_CR1_START | I2C_CR1_ACK;
    if(!waitStatus1()) return false;
    if((I2C1->SR1 & I2C_SR1_SB)==0) return false; //Must read SR1 to clear flag
    I2C1->DR=address;
    if(immediateNak) I2C1->CR1 &= ~I2C_CR1_ACK;
    if(!waitStatus1()) return false;
    if(I2C1->SR1 & I2C_SR1_AF) return false; //Must read SR1 and SR2
    if((I2C1->SR2 & I2C_SR2_MSL)==0) return false;
    return true;
}

bool I2C1Driver::waitStatus1()
{
    error=false;
    waiting=Thread::getCurrentThread();
    I2C1->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITERREN;
    {
        FastInterruptDisableLock dLock;
        while(waiting)
        {
            waiting->IRQwait();
            {
                FastInterruptEnableLock eLock(dLock);
                Thread::yield();
            }
        }
    }
    I2C1->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    return !error;
}

} //namespace miosix
