/***************************************************************************
 *   Copyright (C) 2018-2024 by Terraneo Federico                          *
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
static constexpr unsigned int M=5;   // 25MHz/M=5MHz
#ifdef SYSCLK_FREQ_550MHz
static constexpr unsigned int N=110; // 5MHz*N=550MHz
static constexpr unsigned int P=1;   // 550MHz/P=550MHz
static constexpr unsigned int Q=6;   // 550MHz/P=91.66MHz (<=100MHz for SDMMC)
static constexpr unsigned int R=1;   // 550MHz/P=550MHz
#elif defined(SYSCLK_FREQ_400MHz)
static constexpr unsigned int N=160; // 5MHz*N=800MHz
static constexpr unsigned int P=2;   // 800MHz/P=400MHz
static constexpr unsigned int Q=8;   // 800MHz/P=100MHz (for SDMMC)
static constexpr unsigned int R=2;   // 800MHz/P=400MHz
#else
#error "SYSCLK value not supported!"
#endif
#elif (HSE_VALUE == 8000000)
#ifdef SYSCLK_FREQ_550MHz
static constexpr unsigned int rge=0b01; //2..4MHz
static constexpr unsigned int M=4;   // 8MHz/M=2MHz
static constexpr unsigned int N=275; // 2MHz*N=550MHz
static constexpr unsigned int P=1;   // 550MHz/P=550MHz
static constexpr unsigned int Q=6;   // 550MHz/P=91.66MHz (<=100MHz for SDMMC)
static constexpr unsigned int R=1;   // 550MHz/P=550MHz
#elif defined(SYSCLK_FREQ_400MHz)
static constexpr unsigned int rge=0b01; //2..4MHz
static constexpr unsigned int M=2;   // 8MHz/M=4MHz
static constexpr unsigned int N=200; // 4MHz*N=800MHz
static constexpr unsigned int P=2;   // 800MHz/P=400MHz
static constexpr unsigned int Q=8;   // 800MHz/P=100MHz (for SDMMC)
static constexpr unsigned int R=2;   // 800MHz/P=400MHz
#else
#error "SYSCLK value not supported!"
#endif
#else
#error "HSE value not supported!"
#endif

void startPll()
{
    #ifdef _BOARD_STM32H755ZI_NUCLEO
    //This board is configured to use the SMPS instead of the LDO, and requires
    //hardware rewiring to change that. So use the SMPS.
    //As a consequence, the maximum frequency is 400MHz instead of 480MHz...
    PWR->CR3 &= ~PWR_CR3_LDOEN;
    #endif //_BOARD_STM32H755ZI_NUCLEO

    #ifndef STM32H755xx
    //QUIRK: it looks like VOS can only be set by first setting SCUEN to 0.
    //This isn't documented anywhere in the reference manual.
    //And some chips lack this bit
    PWR->CR3 &= ~PWR_CR3_SCUEN;
    #endif

    //In the STM32H7 DVFS was introduced (chapter 6, Power control)
    //Switch to highest possible voltage to run at the max freq
    #ifdef SYSCLK_FREQ_550MHz
    PWR->D3CR = 0b00<<PWR_D3CR_VOS_Pos; //VOS0
    while((PWR->D3CR & PWR_D3CR_VOSRDY)==0) ; //Wait
    RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
    RCC_SYNC();
    SYSCFG->UR18 |= SYSCFG_UR18_CPU_FREQ_BOOST; //TODO: docs say it's read-only??
    #else
    PWR->D3CR = 0b11<<PWR_D3CR_VOS_Pos; //VOS1
    while((PWR->D3CR & PWR_D3CR_VOSRDY)==0) ; //Wait
    #endif
    
    //Enable HSE oscillator
    RCC->CR |= RCC_CR_HSEON;
    while((RCC->CR & RCC_CR_HSERDY)==0) ; //Wait
    
    //Start the PLL
    RCC->PLLCKSELR |= M<<RCC_PLLCKSELR_DIVM1_Pos
                    | RCC_PLLCKSELR_PLLSRC_HSE; //HSE selected as PLL source

    RCC->PLL1DIVR = (R-1)<<RCC_PLL1DIVR_R1_Pos
                  | (Q-1)<<RCC_PLL1DIVR_Q1_Pos
                  | (P-1)<<RCC_PLL1DIVR_P1_Pos
                  | (N-1)<<RCC_PLL1DIVR_N1_Pos;

    RCC->PLLCFGR |= rge<<RCC_PLLCFGR_PLL1RGE_Pos // Pll ref clock range
                  | RCC_PLLCFGR_DIVP1EN   // Enable output P
                  | RCC_PLLCFGR_DIVQ1EN   // Enable output Q
                  | RCC_PLLCFGR_DIVR1EN;  // Enable output R
    RCC->CR |= RCC_CR_PLL1ON;
    while((RCC->CR & RCC_CR_PLL1RDY)==0) ; //Wait
    
    //Before increasing the fequency set dividers 
    RCC->D1CFGR = RCC_D1CFGR_D1CPRE_DIV1  //CPU clock /1
                | RCC_D1CFGR_D1PPRE_DIV2  //D1 APB3   /2
                | RCC_D1CFGR_HPRE_DIV2;   //D1 AHB    /2
    RCC->D2CFGR = RCC_D2CFGR_D2PPRE2_DIV2 //D2 APB2   /2
                | RCC_D2CFGR_D2PPRE1_DIV2;//D2 APB1   /2
    RCC->D3CFGR = RCC_D3CFGR_D3PPRE_DIV2; //D3 APB4   /2
    
    //And increase FLASH wait states
    #ifdef SYSCLK_FREQ_550MHz
    //Only STM32H723xx supports 550MHz
    #ifdef STM32H723xx
    FLASH->ACR = 0b11<<FLASH_ACR_WRHIGHFREQ_Pos | FLASH_ACR_LATENCY_3WS;
    #else
    #error "Unsupported frequency for this MCU"
    #endif
    #elif defined(SYSCLK_FREQ_400MHz)
    //At 400MHz all MCUs require 2 wait states
    FLASH->ACR = 0b10<<FLASH_ACR_WRHIGHFREQ_Pos | FLASH_ACR_LATENCY_2WS;
    #endif
    //Finally, increase frequency
    RCC->CFGR |= RCC_CFGR_SW_PLL1;
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1) ; //Wait
}
