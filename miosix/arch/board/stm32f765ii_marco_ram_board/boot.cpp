/***************************************************************************
 *   Copyright (C) 2024 by Daniele Cattaneo                                *
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

#include <cmath>
#include "interfaces/arch_registers.h"
#include "interfaces/delays.h"
#include "interfaces/gpio.h"
#include "board_settings.h"

namespace miosix {

enum SDRAMModeReg {
    BurstLenPos             = 0,
    BurstLenMask            = 7<<BurstLenPos,
    BurstLen1               = 0<<BurstLenPos,
    BurstLen2               = 1<<BurstLenPos,
    BurstLen4               = 2<<BurstLenPos,
    BurstLen8               = 3<<BurstLenPos,
    BurstLenPage            = 7<<BurstLenPos,
    BurstTypePos            = 3,
    BurstTypeMask           = 1<<BurstTypePos,
    BurstTypeSequential     = 0<<BurstTypePos,
    BurstTypeInterleave     = 1<<BurstTypePos,
    LatencyPos              = 4,
    LatencyMask             = 7<<LatencyPos,
    Latency2                = 2<<LatencyPos,
    Latency3                = 3<<LatencyPos,
    ModePos                 = 7,
    ModeMask                = 3<<ModePos,
    ModeNormal              = 0<<ModePos,
    WriteBurstModePos       = 9,
    WriteBurstModeMask      = 1<<WriteBurstModePos,
    WriteBurstModeProg      = 0<<WriteBurstModePos,
    WriteBurstModeSingle    = 1<<WriteBurstModePos,
};

template <int div, int t_rcd_ns, int t_rp_ns, int t_wr_ns,
    int t_rc_ns, int t_ras_ns, int t_xsr_ns, int t_mrd_ns>
static constexpr uint32_t sdramSDTR() noexcept
{
    constexpr float hclk_mhz = cpuFrequency/1000000;
    constexpr float t_ns = 1000.0f / (hclk_mhz / (float)div);
    constexpr int sdtr_trcd = std::ceil((float)t_rcd_ns / t_ns) - 1;
    static_assert(0 <= sdtr_trcd && sdtr_trcd <= 15);
    constexpr int sdtr_trp  = std::ceil((float)t_rp_ns  / t_ns) - 1;
    static_assert(0 <= sdtr_trp && sdtr_trp <= 15);
    constexpr int sdtr_twr  = std::ceil((float)t_wr_ns  / t_ns) - 1;
    static_assert(0 <= sdtr_twr && sdtr_twr <= 15);
    constexpr int sdtr_trc  = std::ceil((float)t_rc_ns  / t_ns) - 1;
    static_assert(0 <= sdtr_trc && sdtr_trc <= 15);
    constexpr int sdtr_tras = std::ceil((float)t_ras_ns / t_ns) - 1;
    static_assert(0 <= sdtr_tras && sdtr_tras <= 15);
    constexpr int sdtr_txsr = std::ceil((float)t_xsr_ns / t_ns) - 1;
    static_assert(0 <= sdtr_txsr && sdtr_txsr <= 15);
    constexpr int sdtr_tmrd = std::ceil((float)t_mrd_ns / t_ns) - 1;
    static_assert(0 <= sdtr_tmrd && sdtr_tmrd <= 15);
    return sdtr_trcd << FMC_SDTR1_TRCD_Pos
        | sdtr_trp  << FMC_SDTR1_TRP_Pos  
        | sdtr_twr  << FMC_SDTR1_TWR_Pos  
        | sdtr_trc  << FMC_SDTR1_TRC_Pos  
        | sdtr_tras << FMC_SDTR1_TRAS_Pos 
        | sdtr_txsr << FMC_SDTR1_TXSR_Pos 
        | sdtr_tmrd << FMC_SDTR1_TMRD_Pos;
}

template <int div, int n_rows, int t_refresh_ms>
constexpr uint32_t sdramSDRTR() noexcept
{
    constexpr float hclk_mhz = cpuFrequency/1000000;
    constexpr float t_us = 1.0f / (hclk_mhz / (float)div);
    constexpr float t_refresh_us = (float)t_refresh_ms * 1000.0f;
    constexpr float t_refreshPerRow_us = t_refresh_us / (float)n_rows;
    constexpr int sdrtr_count = (std::floor(t_refreshPerRow_us / t_us) - 20) - 1;
    static_assert(41 <= sdrtr_count && sdrtr_count <= 0x1FFF);
    return sdrtr_count << FMC_SDRTR_COUNT_Pos;
}

void IRQconfigureSDRAM()
{
    // Enable all gpios used by the FMC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN |
                    RCC_AHB1ENR_GPIOFEN | RCC_AHB1ENR_GPIOGEN |
                    RCC_AHB1ENR_GPIOHEN | RCC_AHB1ENR_GPIOIEN;
    // Enable SYSCFG
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    RCC_SYNC();
    // Enable compensation cell
    SYSCFG->CMPCR = SYSCFG_CMPCR_CMP_PD;
    while (!(SYSCFG->CMPCR & SYSCFG_CMPCR_READY_Msk)) ;
    // Set FMC GPIO speed to 100MHz
    //            port  F  E  D  C  B  A  9  8  7  6  5  4  3  2  1  0
    GPIOD->OSPEEDR = 0b11'11'00'00'00'11'11'11'00'00'00'00'00'00'11'11;
    GPIOE->OSPEEDR = 0b11'11'11'11'11'11'11'11'11'00'00'00'00'00'11'11;
    GPIOF->OSPEEDR = 0b11'11'11'11'11'00'00'00'00'00'11'11'11'11'11'11;
    GPIOG->OSPEEDR = 0b11'00'00'00'00'00'00'11'00'00'11'11'00'11'11'11;
    GPIOH->OSPEEDR = 0b11'11'11'11'11'11'11'11'11'11'11'00'11'11'00'10;
    GPIOI->OSPEEDR = 0b00'00'00'00'00'11'11'00'11'11'11'11'11'11'11'11;
    // Set FMC GPIO to alternate mode
    //            port  F  E  D  C  B  A  9  8  7  6  5  4  3  2  1  0
    GPIOD->MODER   = 0b10'10'00'00'00'10'10'10'00'00'00'00'00'00'10'10;
    GPIOE->MODER   = 0b10'10'10'10'10'10'10'10'10'00'00'00'00'00'10'10;
    GPIOF->MODER   = 0b10'10'10'10'10'00'00'00'00'00'10'10'10'10'10'10;
    GPIOG->MODER   = 0b10'00'00'00'00'00'00'10'00'00'10'10'00'10'10'10;
    GPIOH->MODER   = 0b10'10'10'10'10'10'10'10'10'10'10'00'10'10'00'10;
    GPIOI->MODER   = 0b00'00'00'00'00'10'10'00'10'10'10'10'10'10'10'10;
    // No need to update PUPD as the default of zero is already correct.
    // Set FMC GPIO alternate modes
    //         port   FEDCBA98                    76543210
    GPIOD->AFR[1] = 0xcc000ccc; GPIOD->AFR[0] = 0x000000cc;
    GPIOE->AFR[1] = 0xcccccccc; GPIOE->AFR[0] = 0xc00000cc;
    GPIOF->AFR[1] = 0xccccc000; GPIOF->AFR[0] = 0x00cccccc;
    GPIOG->AFR[1] = 0xc000000c; GPIOG->AFR[0] = 0x00cc0ccc;
    GPIOH->AFR[1] = 0xcccccccc; GPIOH->AFR[0] = 0xccc0cc0c;
    GPIOI->AFR[1] = 0x00000cc0; GPIOI->AFR[0] = 0xcccccccc;

    // Power on FMC
    RCC->AHB3ENR = RCC_AHB3ENR_FMCEN;
    RCC_SYNC();
    // Program memory device features and timings for 8 paralleled
    // IS42S86400D-7TLI chips
    constexpr int clockDiv = 3;
    constexpr int casLatency = 2;
    uint32_t sdcr =
          (3 << FMC_SDCR1_NC_Pos)       // 11 column address bits
        | (2 << FMC_SDCR1_NR_Pos)       // 13 row address bits
        | (2 << FMC_SDCR1_MWID_Pos)     // 32 bit data bus
        | (1 << FMC_SDCR1_NB_Pos)       // 4 internal banks
        | (casLatency << FMC_SDCR1_CAS_Pos)
        | (0 << FMC_SDCR1_WP_Pos)       // write allowed
        | (clockDiv << FMC_SDCR1_SDCLK_Pos) // clock = HCLK / clockDiv
        | (1 << FMC_SDCR1_RBURST_Pos)   // burst mode on/off
        | (0 << FMC_SDCR1_RPIPE_Pos);   // delay after reading
    FMC_Bank5_6->SDCR[0] = sdcr;
    FMC_Bank5_6->SDCR[1] = sdcr;
    uint32_t sdtr = sdramSDTR<
            clockDiv,
            20, // Row to column delay (t_RCD)
            20, // Row precharge delay (t_RP)
            14, // Write to precharge delay (t_WR) (ISSI denotes this as t_DPL)
            70, // Row cycle delay (t_RC)
            49, // Self-refresh time (t_RAS)
            77, // Exit self-refresh delay (t_XSR)
            14  // Load mode register to active (t_MRD)
        >();
    FMC_Bank5_6->SDTR[0] = sdtr;
    FMC_Bank5_6->SDTR[1] = sdtr;
    // Send init command and wait for powerup
    FMC_Bank5_6->SDCMR = (1 << FMC_SDCMR_MODE_Pos) | FMC_SDCMR_CTB1 | FMC_SDCMR_CTB2;
    while (FMC_Bank5_6->SDSR & FMC_SDSR_BUSY) ;
    miosix::delayUs(200); // see RAM datasheet p.21 for wait time
    // Precharge all banks
    FMC_Bank5_6->SDCMR = (2 << FMC_SDCMR_MODE_Pos) | FMC_SDCMR_CTB1 | FMC_SDCMR_CTB2;
    while (FMC_Bank5_6->SDSR & FMC_SDSR_BUSY) ;
    // Issue 8 auto-refresh commands (see RAM datasheet p.21 for number of refreshes)
    FMC_Bank5_6->SDCMR = (3 << FMC_SDCMR_MODE_Pos) | (7 << FMC_SDCMR_NRFS_Pos) | FMC_SDCMR_CTB1 | FMC_SDCMR_CTB2;
    while (FMC_Bank5_6->SDSR & FMC_SDSR_BUSY) ;
    // Issue mode register set command
    uint32_t mrd =
          SDRAMModeReg::BurstLen1
        | SDRAMModeReg::BurstTypeSequential
        | (casLatency << SDRAMModeReg::LatencyPos)
        | SDRAMModeReg::ModeNormal
        | SDRAMModeReg::WriteBurstModeSingle;
    FMC_Bank5_6->SDCMR = (4 << FMC_SDCMR_MODE_Pos) | (mrd << FMC_SDCMR_MRD_Pos) | FMC_SDCMR_CTB1 | FMC_SDCMR_CTB2;
    while (FMC_Bank5_6->SDSR & FMC_SDSR_BUSY) ;
    // Program refresh rate
    FMC_Bank5_6->SDRTR = sdramSDRTR<
            clockDiv,
            8192,   // Number of rows
            64      // Refresh cycle time (ms)
        >();
}

void IRQmemoryAndClockInit()
{
    SystemInit();
    IRQconfigureSDRAM();
}

} // namespace miosix
