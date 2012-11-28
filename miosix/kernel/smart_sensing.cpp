/* 
 * File:   SmartSensing.cpp
 * Author: Alessandro Rizzi
 * 
 * Created on 18 novembre 2012, 12.48
 */

#include "smart_sensing.h"

namespace miosix {
    
    void debugInt(unsigned int i) {

        unsigned int m;
        unsigned int r;
        int revert[16];
        int j = 0;
        r = i;
        while (r > 0) {
            m = r % 10;
            r = (r - m) / 10;
            revert[j++] = m;
        }
        IRQbootlog("|");
        j--;
        for (; j >= 0; j--) {
            switch (revert[j]) {
                case 0:
                    IRQbootlog("0");
                    break;
                case 1:
                    IRQbootlog("1");
                    break;
                case 2:
                    IRQbootlog("2");
                    break;
                case 3:
                    IRQbootlog("3");
                    break;
                case 4:
                    IRQbootlog("4");
                    break;
                case 5:
                    IRQbootlog("5");
                    break;
                case 6:
                    IRQbootlog("6");
                    break;
                case 7:
                    IRQbootlog("7");
                    break;
                case 8:
                    IRQbootlog("8");
                    break;
                case 9:
                    IRQbootlog("9");
                    break;
            }
        }
        IRQbootlog("\r\n");
    }

}
