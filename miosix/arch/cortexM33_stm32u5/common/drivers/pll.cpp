/***************************************************************************
 *   Copyright (C) 2026 by Alain Carlucci                                  *
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
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "interfaces/arch_registers.h"
#include "pll.h"

//Constraints:
//PLL input  between 2MHz   and 16MHz (wide range PLL can't run with <2MHz)
//PLL output between 192MHz and 836MHz(h723/h743) 960MHz(h755)
#if (HSE_VALUE == 25000000)
    static constexpr unsigned int rge=0b10; //4..8MHz
    static constexpr unsigned int M=4;   // 25MHz/(5-1)=5MHz
    static constexpr unsigned int MBoost=2;   // 4 < 25MHz/2^Mboost < 16
    #ifdef SYSCLK_FREQ_160MHz
        static constexpr unsigned int N=32;  // 5MHz*N=160MHz
        static constexpr unsigned int P=1;   // 160MHz/P=160MHz
        static constexpr unsigned int Q=1;   // 160MHz/Q=160MHz
        static constexpr unsigned int R=1;   // 160MHz/R=160MHz
        static constexpr unsigned int VCore=0b11; // Range 1 (Highest)
        static constexpr bool boostEnable=1;  // Yes if fClk > 55MHz
    #elif SYSCLK_FREQ_110MHz
        static constexpr unsigned int N=22;  // 5MHz*N=110MHz
        static constexpr unsigned int P=1;   // 110MHz/P=110MHz
        static constexpr unsigned int Q=1;   // 110MHz/Q=110MHz
        static constexpr unsigned int R=1;   // 110MHz/R=110MHz
        static constexpr unsigned int VCore=0b11; // Range 2
        static constexpr bool boostEnable=1;  // Yes if fClk > 55MHz
    #elif SYSCLK_FREQ_48MHz
        static constexpr unsigned int N=48;  // 5MHz*N=240MHz
        static constexpr unsigned int P=5;   // 110MHz/P=48MHz
        static constexpr unsigned int Q=5;   // 110MHz/Q=48MHz
        static constexpr unsigned int R=5;   // 110MHz/R=48MHz
        static constexpr unsigned int VCore=0b11; // Range 2
        static constexpr bool boostEnable=1;  // Yes if fClk > 55MHz
    #elif SYSCLK_FREQ_24MHz
        static constexpr unsigned int N=24;  // 5MHz*N=120MHz
        static constexpr unsigned int P=5;   // 110MHz/P=24MHz
        static constexpr unsigned int Q=5;   // 110MHz/Q=24MHz
        static constexpr unsigned int R=5;   // 110MHz/R=24MHz
        static constexpr unsigned int VCore=0b11; // Range 2
        static constexpr bool boostEnable=1;  // Yes if fClk > 55MHz
    #else
        #error "SYSCLK value not supported!"
    #endif
#else
    #error "HSE value not supported!"
#endif

void startPll()
{
    PWR->CR3 &= ~PWR_CR3_REGSEL;
    RCC->AHB3ENR |= RCC_AHB3ENR_PWREN;
    RCC_SYNC();

    // 1. Set voltage range
    #if defined(SYSCLK_FREQ_160MHz) or defined(SYSCLK_FREQ_110MHz) or defined(SYSCLK_FREQ_48MHz) or defined(SYSCLK_FREQ_24MHz)
        //Enable HSE oscillator
        RCC->CR |= RCC_CR_HSEON;
        while((RCC->CR & RCC_CR_HSERDY)==0) ;

        // PLL must be enabled before VOSR

        RCC->PLL1DIVR = (R-1)<<RCC_PLL1DIVR_PLL1R_Pos
                    | (Q-1)<<RCC_PLL1DIVR_PLL1Q_Pos
                    | (P-1)<<RCC_PLL1DIVR_PLL1P_Pos
                    | (N-1)<<RCC_PLL1DIVR_PLL1N_Pos;

        RCC->PLL1CFGR = RCC_PLL1CFGR_PLL1REN
                    | RCC_PLL1CFGR_PLL1QEN
                    | RCC_PLL1CFGR_PLL1PEN
                    | (M << RCC_PLL1CFGR_PLL1M_Pos)
                    | (MBoost << RCC_PLL1CFGR_PLL1MBOOST_Pos) // 4 < 25/4 < 16
                    | (rge << RCC_PLL1CFGR_PLL1RGE_Pos)
                    | (VCore << RCC_PLL1CFGR_PLL1SRC_Pos); // HSE

        RCC->CR |= RCC_CR_PLL1ON;
        while((RCC->CR & RCC_CR_PLL1RDY) == 0) ;

        ////// Highest frequency (Range 1) [boost needed if tgt freq >= 55MHz)
        PWR->VOSR = (0b11<<PWR_VOSR_VOS_Pos);
        while((PWR->VOSR & PWR_VOSR_VOSRDY)==0);

        if (boostEnable)
        {
            PWR->VOSR |= (PWR_VOSR_BOOSTEN);
            while ((PWR->VOSR & PWR_VOSR_BOOSTRDY) == 0);
        }
#else
#error
    #endif
    
    // Increase FLASH wait states (RM 7.3.3, Table 54)

    #ifdef SYSCLK_FREQ_160MHz
        // 160MHz with Vcore range 1 needs 4 wait states
        FLASH->ACR = FLASH_ACR_PRFTEN | (FLASH_ACR_LATENCY_4WS & 0x0f);
    #elif SYSCLK_FREQ_110MHz
        // Assuming VCore Range 2
        FLASH->ACR = FLASH_ACR_PRFTEN | (FLASH_ACR_LATENCY_3WS & 0x0f);
    #elif SYSCLK_FREQ_48MHz
        // Assuming VCore Range 3
        FLASH->ACR = FLASH_ACR_PRFTEN | (FLASH_ACR_LATENCY_1WS & 0x0f);
    #elif SYSCLK_FREQ_24MHz
        // Assuming VCore Range 4
        FLASH->ACR = FLASH_ACR_PRFTEN | (FLASH_ACR_LATENCY_1WS & 0x0f);
    #else
        #error Not implemented
    #endif

    RCC->CFGR1 |= 0b11 << RCC_CFGR1_SW_Pos;
    while((RCC->CFGR1 & RCC_CFGR1_SWS) != (0b11 << RCC_CFGR1_SWS_Pos)) ; //Wait

    SystemCoreClockUpdate();
}
