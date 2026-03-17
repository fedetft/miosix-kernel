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

#include "board_settings.h"
#include "interfaces/arch_registers.h"
#include "pll.h"

using namespace miosix;

struct RCCPLLConfig
{
    OscillatorType oscType;
    unsigned int hse, sysclk;
    unsigned int rge, M, MBoost, N, P, Q, R, VCore;
    bool boostEnable;
};

//Constraints:
//PLL input  between 2MHz   and 16MHz (wide range PLL can't run with <2MHz)
//PLL output between 192MHz and 836MHz(h723/h743) 960MHz(h755)

static constexpr RCCPLLConfig pllConfigs[] = {
    {
        .oscType=OscillatorType::HSE, .hse=25000000, .sysclk=160000000,
        .rge=0b10,      //4..8MHz
        .M=4,           // 25MHz/(5-1)=5MHz
        .MBoost=2,      // 4 < 25MHz/2^Mboost < 16
        .N=32,          // 5MHz*N=160MHz
        .P=1,           // 160MHz/P=160MHz
        .Q=1,           // 160MHz/Q=160MHz
        .R=1,           // 160MHz/R=160MHz
        .VCore=0b11,    // Range 1 (Highest)
        .boostEnable=1, // Yes if fClk > 55MHz
    },
    {
        .oscType=OscillatorType::HSE, .hse=25000000, .sysclk=110000000,
        .rge=0b10,      //4..8MHz
        .M=4,           // 25MHz/(5-1)=5MHz
        .MBoost=2,      // 4 < 25MHz/2^Mboost < 16
        .N=22,          // 5MHz*N=110MHz
        .P=1,           // 110MHz/P=110MHz
        .Q=1,           // 110MHz/Q=110MHz
        .R=1,           // 110MHz/R=110MHz
        .VCore=0b10,    // Range 2
        .boostEnable=1, // Yes if fClk > 55MHz
    },
    {
        .oscType=OscillatorType::HSE, .hse=25000000, .sysclk=48000000,
        .rge=0b10,      //4..8MHz
        .M=4,           // 25MHz/(5-1)=5MHz
        .MBoost=2,      // 4 < 25MHz/2^Mboost < 16
        .N=48,          // 5MHz*N=240MHz
        .P=5,           // 110MHz/P=48MHz
        .Q=5,           // 110MHz/Q=48MHz
        .R=5,           // 110MHz/R=48MHz
        .VCore=0b10,    // Range 2
        .boostEnable=0  // Yes if fClk > 55MHz
    },
    {
        .oscType=OscillatorType::HSE, .hse=25000000, .sysclk=24000000,
        .rge=0b10,      //4..8MHz
        .M=4,           // 25MHz/(5-1)=5MHz
        .MBoost=2,      // 4 < 25MHz/2^Mboost < 16
        .N=24,          // 5MHz*N=120MHz
        .P=5,           // 110MHz/P=24MHz
        .Q=5,           // 110MHz/Q=24MHz
        .R=5,           // 110MHz/R=24MHz
        .VCore=0b10,    // Range 2
        .boostEnable=0, // Yes if fClk > 55MHz
    },
    {
        .oscType=OscillatorType::HSI, .hse=0, .sysclk=48000000,
        .rge=0b10,      //4..8MHz
        .M=3,           // 16MHz/(4-1)=4MHz
        .MBoost=1,      // 4 < 25MHz/2^Mboost < 16
        .N=12,          // 4MHz*N=48MHz
        .P=1,           // 48MHz/P=48MHz
        .Q=1,           // 48MHz/Q=48MHz
        .R=1,           // 48MHz/R=48MHz
        .VCore=0b01,    // Range 3
        .boostEnable=0, // Yes if fClk > 55MHz
    }
};

template<unsigned int N>
constexpr RCCPLLConfig findPllConfig()
{
    static_assert(N<sizeof(pllConfigs)/sizeof(RCCPLLConfig), "Unsupported sysclk");
    if constexpr(pllConfigs[N].oscType==oscillatorType &&
                 (pllConfigs[N].oscType!=OscillatorType::HSE || pllConfigs[N].hse==hseFrequency) &&
                 pllConfigs[N].sysclk==cpuFrequency) return pllConfigs[N];
    else return findPllConfig<N+1>();
}

void startPll()
{
    PWR->CR3 &= ~PWR_CR3_REGSEL;
    RCC->AHB3ENR |= RCC_AHB3ENR_PWREN;
    RCC_SYNC();

    RCCPLLConfig cfg=findPllConfig<0>();

    // 1. Set voltage range
    if (oscillatorType==OscillatorType::HSE)
    {
        //Enable HSE oscillator
        RCC->CR |= RCC_CR_HSEON;
        while((RCC->CR & RCC_CR_HSERDY)==0) ;

        // PLL must be enabled before VOSR

        RCC->PLL1DIVR = (cfg.R-1)<<RCC_PLL1DIVR_PLL1R_Pos
                    | (cfg.Q-1)<<RCC_PLL1DIVR_PLL1Q_Pos
                    | (cfg.P-1)<<RCC_PLL1DIVR_PLL1P_Pos
                    | (cfg.N-1)<<RCC_PLL1DIVR_PLL1N_Pos;

        RCC->PLL1CFGR = RCC_PLL1CFGR_PLL1REN
                    | RCC_PLL1CFGR_PLL1QEN
                    | RCC_PLL1CFGR_PLL1PEN
                    | (cfg.M << RCC_PLL1CFGR_PLL1M_Pos)
                    | (cfg.MBoost << RCC_PLL1CFGR_PLL1MBOOST_Pos) // 4 < 25/4 < 16
                    | (cfg.rge << RCC_PLL1CFGR_PLL1RGE_Pos)
                    | (0b11 << RCC_PLL1CFGR_PLL1SRC_Pos); // HSE

        RCC->CR |= RCC_CR_PLL1ON;
        while((RCC->CR & RCC_CR_PLL1RDY) == 0) ;
    } else {
        //Enable HSI oscillator
        RCC->CR |= RCC_CR_HSION;
        while((RCC->CR & RCC_CR_HSIRDY)==0) ;

        // PLL must be enabled before VOSR

        RCC->PLL1DIVR = (cfg.R-1)<<RCC_PLL1DIVR_PLL1R_Pos
                    | (cfg.Q-1)<<RCC_PLL1DIVR_PLL1Q_Pos
                    | (cfg.P-1)<<RCC_PLL1DIVR_PLL1P_Pos
                    | (cfg.N-1)<<RCC_PLL1DIVR_PLL1N_Pos;

        RCC->PLL1CFGR = RCC_PLL1CFGR_PLL1REN
                    | RCC_PLL1CFGR_PLL1QEN
                    | RCC_PLL1CFGR_PLL1PEN
                    | (cfg.M << RCC_PLL1CFGR_PLL1M_Pos)
                    | (cfg.MBoost << RCC_PLL1CFGR_PLL1MBOOST_Pos) // 4 < 25/4 < 16
                    | (cfg.rge << RCC_PLL1CFGR_PLL1RGE_Pos)
                    | (0b10 << RCC_PLL1CFGR_PLL1SRC_Pos); // HSI16

        RCC->CR |= RCC_CR_PLL1ON;
        while((RCC->CR & RCC_CR_PLL1RDY) == 0) ;
    }

    // Highest frequency (Range 1) [boost needed if tgt freq >= 55MHz)
    PWR->VOSR = (cfg.VCore<<PWR_VOSR_VOS_Pos);
    while((PWR->VOSR & PWR_VOSR_VOSRDY)==0);

    if (cfg.boostEnable)
    {
        PWR->VOSR |= (PWR_VOSR_BOOSTEN);
        while ((PWR->VOSR & PWR_VOSR_BOOSTRDY) == 0);
    }
    
    // Increase FLASH wait states (RM 7.3.3, Table 54)
    constexpr unsigned int ws=
        cpuFrequency<=24000000 ? FLASH_ACR_LATENCY_1WS : // Assuming VCore Range 4
        cpuFrequency<=48000000 ? FLASH_ACR_LATENCY_1WS : // Assuming VCore Range 3
        cpuFrequency<=110000000 ? FLASH_ACR_LATENCY_3WS : // Assuming VCore Range 2
        FLASH_ACR_LATENCY_4WS; // (160MHz) Assuming VCore Range 1
    FLASH->ACR = FLASH_ACR_PRFTEN | (ws & 0x0f);

    RCC->CFGR1 |= 0b11 << RCC_CFGR1_SW_Pos;
    while((RCC->CFGR1 & RCC_CFGR1_SWS) != (0b11 << RCC_CFGR1_SWS_Pos)) ; //Wait

    SystemCoreClockUpdate();
}
