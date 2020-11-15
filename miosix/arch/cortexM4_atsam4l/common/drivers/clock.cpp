/***************************************************************************
 *   Copyright (C) 2020 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the suorce code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "clock.h"
#include "interfaces/arch_registers.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//NOTE: atsam4l start at reset with a 115kHz oscillator, that is way too slow
// here we configure the internal RC oscillator to 12MHz
//TODO: support more clock options

unsigned int SystemCoreClock = 12000000;

void SystemInit()
{
    SCIF->SCIF_UNLOCK=0xaa<<24 | SCIF_RCFASTCFG_OFFSET;
    SCIF->SCIF_RCFASTCFG |= (1<<9) | SCIF_RCFASTCFG_EN; //12MHz
    PM->PM_UNLOCK=0xaa<<24 | PM_MCCTRL_OFFSET;
    PM->PM_MCCTRL=PM_MCCTRL_MCSEL_RCFAST;
}

#ifdef __cplusplus
}
#endif //__cplusplus

void start32kHzOscillator()
{
//     iprintf("Starting 32kHz XTAL... ");
//     fflush(stdout);

    //NOTE: at least with the 32kHz crystal I've tested (CL=12.5pF), this
    //oscillator has a very noticeable jitter. Triggering with a scope on the
    //rising edge, you can see it by zooming on the falling edge. Using the
    //maximum current of 425nA reduced the jitter, but it is still ~200ns!
    //Amplitude controlled mode is worse, don't use it.
    BSCIF->BSCIF_OSCCTRL32 = BSCIF_OSCCTRL32_STARTUP(4)  //64K cycles startup
                           | BSCIF_OSCCTRL32_SELCURR(15) //425nA (max)
                           | BSCIF_OSCCTRL32_MODE(1)     //Crystal mode
                           | BSCIF_OSCCTRL32_EN1K
                           | BSCIF_OSCCTRL32_EN32K
                           | BSCIF_OSCCTRL32_OSC32EN;
    while((BSCIF->BSCIF_PCLKSR & BSCIF_PCLKSR_OSC32RDY) == 0) ;
//     puts("Ok");

//     //Output OSC32K on PA2/GCLK0 for measurement purpose
//     SCIF->SCIF_GCCTRL[0].SCIF_GCCTRL = SCIF_GCCTRL_OSCSEL(1) //Output OSC32K
//                                      | SCIF_GCCTRL_CEN;
//     using gclk0 = Gpio<GPIOA_BASE,2>;
//     gclk0::mode(Mode::ALTERNATE);
//     gclk0::alternateFunction('A');
}
