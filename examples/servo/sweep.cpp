
#include <stdio.h>
#include "drivers/servo_stm32.h"

using namespace miosix;

int main()
{
    SynchronizedServo& servo=SynchronizedServo::instance();
    for(int i=0;i<4;i++) servo.enable(i);
    servo.start();
    for(float f=0.0f;;f+=0.01f)
    {
        servo.waitForCycleBegin();
        ledOn();
        for(int i=0;i<4;i++) servo.setPosition(i,f);
        if(f>=1.0f) f=0.0f;
        ledOff();
    }
}
