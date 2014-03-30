
#include <stdio.h>
#include "drivers/servo_stm32.h"

using namespace miosix;

int main()
{
    SynchronizedServo& servo=SynchronizedServo::instance();
    for(int i=0;i<4;i++) servo.enable(i);
    servo.start();
    for(;;)
    {
        printf("id (0-3), val (0.0-1.0)?\n");
        int id;
        float val;
        scanf("%d %f",&id,&val);
        servo.setPosition(id,val);
    }
}
