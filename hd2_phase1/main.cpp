/*
 * Phase-1 CONTROL — pure busy-wait blink (NO Thread::sleep), the proven demo.
 * If both LEDs blink, the board/kernel/build/preemption are all fine, and the
 * earlier "no LED" was caused specifically by Thread::sleep (likely a fault,
 * not just a hang). If this is ALSO dead, the kernel/build regressed.
 */

#include "miosix.h"

using namespace miosix;

static volatile unsigned int * const GPIOB_DR =
    reinterpret_cast<volatile unsigned int*>(0x14100000u);

static void greenThread(void*)
{
    for(;;) { *GPIOB_DR ^= (1u<<0); for(volatile int i=0;i<1500000;i++){} }
}

int main()
{
    *GPIOB_DR &= ~((1u<<0)|(1u<<1));
    Thread::create(greenThread, 512);
    for(;;) { *GPIOB_DR ^= (1u<<1); for(volatile int i=0;i<5000000;i++){} }
}
