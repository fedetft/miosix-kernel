
//Example to drive an 8-digit common cathode multiplexed 7-segment LED display.
//Add current limiting resistors and digit drivers as needed.
//Written for stm32vldiscovery, change GPIOs and the line mentioning AFIO->MAPR
//if targeting another board.

#include <string.h>
#include <miosix.h>

using namespace miosix;

typedef Gpio<PB, 3> segmentA;
typedef Gpio<PB, 4> segmentB;
typedef Gpio<PB, 5> segmentC;
typedef Gpio<PB, 6> segmentD;
typedef Gpio<PB, 7> segmentE;
typedef Gpio<PB, 8> segmentF;
typedef Gpio<PB, 9> segmentG;
typedef Gpio<PB,10> segmentDP;
typedef Gpio<PD, 2> digit0;
typedef Gpio<PC,12> digit1;
typedef Gpio<PC,11> digit2;
typedef Gpio<PC,10> digit3;
typedef Gpio<PA,15> digit4;
typedef Gpio<PA, 3> digit5;
typedef Gpio<PA, 2> digit6;
typedef Gpio<PA, 1> digit7;

static void outSegment(unsigned char segment)
{
    #define S2G(x,y,z) if(y & (1<<z)) x::high(); else x::low()
    S2G(segmentA, segment,0);
    S2G(segmentB, segment,1);
    S2G(segmentC, segment,2);
    S2G(segmentD, segment,3);
    S2G(segmentE, segment,4);
    S2G(segmentF, segment,5);
    S2G(segmentG, segment,6);
    S2G(segmentDP,segment,7);
    #undef S2G
}

static unsigned char digits[8];

static void displayThread(void *)
{
    #define CONFIG(x) x::high(); x::mode(Mode::OUTPUT)
    {
        FastGlobalIrqLock dLock;
        AFIO->MAPR=1<<25;//To free PB3 and PB4 from JTAG in stm32vldiscovery
        CONFIG(digit0);
        CONFIG(digit1);
        CONFIG(digit2);
        CONFIG(digit3);
        CONFIG(digit4);
        CONFIG(digit5);
        CONFIG(digit6);
        CONFIG(digit7);
        CONFIG(segmentA);
        CONFIG(segmentB);
        CONFIG(segmentC);
        CONFIG(segmentD);
        CONFIG(segmentE);
        CONFIG(segmentF);
        CONFIG(segmentG);
        CONFIG(segmentDP);
    }
    #undef CONFIG
    #define DIGIT(x,y) \
        outSegment(digits[y]); x::low(); Thread::sleep(2); x::high()
    for(;;)
    {
        DIGIT(digit0,0);
        DIGIT(digit1,1);
        DIGIT(digit2,2);
        DIGIT(digit3,3);
        DIGIT(digit4,4);
        DIGIT(digit5,5);
        DIGIT(digit6,6);
        DIGIT(digit7,7);
    }
    #undef DIGIT
}

static const unsigned char digitTable[]=
{
    0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f
};

static void number(int x)
{
    for(int i=0;i<8;i++)
    {
        digits[i]=digitTable[x % 10];
        x/=10;
    }
}

int main()
{
    memset(digits,0,sizeof(digits));
    Thread::create(displayThread,STACK_MIN,DEFAULT_PRIORITY,nullptr,Thread::DETACHED);
    for(int i=0;;i++)
    {
        number(i);
        Thread::sleep(10);
    }
}
