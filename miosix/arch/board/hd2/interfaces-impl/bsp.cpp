/***************************************************************************
 *   Board support package for the Ailunce HD2 (HR_C7000 / CK803S).         *
 *   GPL v2+ with the Miosix linking exception.                            *
 *                                                                          *
 *   Provides: IRQmemoryAndClockInit (early PLL bring-up, called from the   *
 *   cpu Reset_Handler before .data/.bss), IRQbspInit (board peripherals),  *
 *   bspInit2, shutdown, reboot.                                            *
 *                                                                          *
 *   PHASE 1: a polled-TX UART0 debug console (hd2_dbg_puts, 57600 8N1) is  *
 *   wired for bring-up tracing; bring-up is also observable via the LEDs   *
 *   (bsp_impl.h ledOn/ledOff = GPIOB.0 green). Full Miosix stdio/iprintf   *
 *   routing through a Device is a later step.                              *
 *                                                                          *
 *   NOTE: after a YMODEM flash the IAP does NOT reliably leave DFU on the  *
 *   '3' boot byte (radio sits solid-red); a cold power-cycle (battery      *
 *   pull) boots the freshly flashed image. Capture serial across that.     *
 ***************************************************************************/

#include <sys/ioctl.h>
#include "interfaces/bsp.h"
#include "interfaces_private/bsp_private.h"
#include "interfaces/poweroff.h"
#include "interfaces/arch_registers.h"
#include "kernel/lock.h"
#include "miosix_settings.h"
#include "../../../../../openrtx_hd2/hd2_crumb.h"  //hard-lock forensics block

//Previous life's breadcrumbs (see hd2_crumb.h) -- filled by IRQbspInit.
hd2_crumb_t hd2_crumb_prev;

namespace miosix {

//-----------------------------------------------------------------------------
// Raw polled-TX serial debug on UART0 (0x14030000, the loader/debug UART the
// flashing bridge reads at 57600). DesignWare 16550: THR/DLL +0x00, DLH/IER
// +0x04, FCR +0x08, LCR +0x0c, LSR +0x14. Deliberately NOT routed through the
// Miosix Device/stdio/iprintf path (that hangs early boot here); this is a
// bare hd2_dbg_puts() the app/board code can call directly. Bounded spin so a
// mis-clocked/stuck UART can never hang. We do NOT touch the divisor — the IAP
// already set UART0 to 57600 and we inherit it (baud confirmed empirically).
//-----------------------------------------------------------------------------
#define HD2_UART0(off)  (*(volatile unsigned int*)(0x14030000u+(off)))
#define UART0_THR       HD2_UART0(0x00u)
#define UART0_DLL       HD2_UART0(0x00u)
#define UART0_DLH       HD2_UART0(0x04u)
#define UART0_FCR       HD2_UART0(0x08u)
#define UART0_LCR       HD2_UART0(0x0cu)
#define UART0_LSR       HD2_UART0(0x14u)
#define UART_LSR_THRE   0x20u   //THR empty: ok to write

} //namespace miosix

//clk_init_pll halves the UART ref clock (84->42 MHz), so the IAP's divisor is
//now wrong. Reprogram for 57600 8N1 at 42 MHz: DLL = 42e6/(16*57600) ~= 46
//(the OpenRTX-proven value, project_hd2_clock_init). MUST run AFTER
//IRQmemoryAndClockInit (clk_init_pll), which IRQbspInit does.
extern "C" void hd2_dbg_init()
{
    UART0_LCR=0x80u;        //DLAB=1
    UART0_DLL=46u;
    UART0_DLH=0u;
    UART0_LCR=0x03u;        //8N1, DLAB=0
    UART0_FCR=0x07u;        //enable + reset both FIFOs
}

extern "C" void hd2_dbg_putc(char c)
{
    for(unsigned int g=0; g<200000u && (UART0_LSR & UART_LSR_THRE)==0u; ++g) {}
    UART0_THR=static_cast<unsigned char>(c);
}

extern "C" void hd2_dbg_puts(const char *s)
{
    while(*s) hd2_dbg_putc(*s++);
}

namespace miosix {

//-----------------------------------------------------------------------------
// HD2 SOCSYS clock/PLL registers (0x11000000) — the subset clk_init_pll needs.
// Named per hd2_regs.h (no magic numbers). Mirrors the HW-verified vendor
// sequence ported in OpenRTX platform.c (project_hd2_clock_init).
//-----------------------------------------------------------------------------
#define HD2_SOCSYS_BASE   0x11000000u
#define HD2_SOCSYS(off)   (*(volatile unsigned int*)(HD2_SOCSYS_BASE+(off)))
#define SOCSYS_CLK_CTRL   HD2_SOCSYS(0x04u)   // [31]pll_ld(RO) [30]clk_rdy(RO) [3]bclk_sel [2]aclk_sel [1:0]re_cfg
#define SOCSYS_APLL       HD2_SOCSYS(0x08u)
#define SOCSYS_REG10      HD2_SOCSYS(0x10u)   // BPLL
#define SOCSYS_CLKDIV_18  HD2_SOCSYS(0x18u)
#define SOCSYS_CLKDIV_1C  HD2_SOCSYS(0x1cu)
#define SOCSYS_CLKDIV_20  HD2_SOCSYS(0x20u)
#define SOCSYS_REG24      HD2_SOCSYS(0x24u)
#define SOCSYS_REG28      HD2_SOCSYS(0x28u)
#define SOCSYS_REG2C      HD2_SOCSYS(0x2cu)   // gated-clock enable
#define SOCSYS_REG30      HD2_SOCSYS(0x30u)

#define CLK_CTRL_PLL_LD        0x80000000u
#define CLK_CTRL_CLK_RDY       0x40000000u
#define CLK_CTRL_ACLK_WORK_SEL 0x04u
#define CLK_CTRL_BCLK_WORK_SEL 0x08u
#define CLK_CTRL_RE_CFG        0x03u

#define SOCSYS_APLL_CFG        0x05040eb2u
#define SOCSYS_CLKDIV_18_CFG   0x100a0c0cu
#define SOCSYS_CLKDIV_1C_CFG   0x2e002900u
#define SOCSYS_CLKDIV_20_CFG   0xa5771177u
#define SOCSYS_REG2C_CFG       0xfff0ff3cu

// HD2 GPIO bank B (DW_apb_gpio): DR +0x00, DDR +0x04. LEDs + power latch.
#define HD2_GPIOB_BASE    0x14100000u
#define HD2_GPIOB_DR      (*(volatile unsigned int*)(HD2_GPIOB_BASE+0x00u))
#define HD2_GPIOB_DDR     (*(volatile unsigned int*)(HD2_GPIOB_BASE+0x04u))
#define HD2_LED_GREEN     (1u<<0)    // PTB0, active-high
#define HD2_LED_RED       (1u<<1)    // PTB1, active-high
#define HD2_PWR_HOLD      (1u<<13)   // PTB13: power self-latch (HIGH=hold, LOW=cut)

//-----------------------------------------------------------------------------

static void clkBusyDelay()
{
    //Vendor FUN_00030ab8(100); ~25k cycles is generous at the ck803s default.
    for(volatile unsigned int i=0;i<25000u;++i) { }
}

/// Wait for a CLK_CTRL flag asserted for >10 consecutive reads (manual ch.04
/// steps 8 & 10). Returns true if confirmed stable, false on timeout.
static bool clkWaitBitStable(unsigned int mask)
{
    unsigned int consecutive=0;
    for(unsigned int guard=0;guard<4000u;++guard)
    {
        if((SOCSYS_CLK_CTRL & mask)!=0u) { if(++consecutive>10u) return true; }
        else consecutive=0;
    }
    return false;
}

void IRQmemoryAndClockInit()
{
    //--- Disable the watchdog FIRST. The Dahua IAP enables the DW_apb_wdt
    //(0x14010000); a standalone firmware that never "knocks" it gets reset-
    //looped (continuous IAP re-run + UART junk, LEDs stuck early). Unlock
    //(WDG_LOCK=0x5ada7200) then WDG_EN=0 (manual §4.10, Register Table 8).
    *reinterpret_cast<volatile unsigned int*>(0x14010000u)=0x5ada7200u; //unlock
    *reinterpret_cast<volatile unsigned int*>(0x14010010u)=0u;          //WDG_EN=0

    //Port of vendor V2.1.3 FUN_00030b6c (project_hd2_clock_init). Brings the
    //APLL/BPLL up and switches the baseband+SoC clocks onto them — without this
    //the DW timer/peripherals are at the IAP-default clock and the measured
    //42 MHz timebase (HD2_TIMER_HZ) does not hold. Pure register writes; safe
    //to run before .data/.bss init. No external RAM to bring up (all internal).
    SOCSYS_APLL     =SOCSYS_APLL_CFG;
    SOCSYS_CLKDIV_18=SOCSYS_CLKDIV_18_CFG;
    SOCSYS_CLKDIV_1C=SOCSYS_CLKDIV_1C_CFG;
    SOCSYS_CLKDIV_20=SOCSYS_CLKDIV_20_CFG;

    SOCSYS_REG30=2u;
    SOCSYS_REG10=(SOCSYS_REG10 & 0xff000000u) | 0x712u;
    SOCSYS_REG28=6u;
    SOCSYS_REG24=(SOCSYS_REG24 & 0xfffffff0u) | 4u;

    SOCSYS_CLK_CTRL|=CLK_CTRL_RE_CFG;          //engage both PLLs
    clkWaitBitStable(CLK_CTRL_PLL_LD);         //confirm PLL lock before switching

    SOCSYS_CLK_CTRL&=~CLK_CTRL_ACLK_WORK_SEL;  //baseband -> APLL
    clkBusyDelay();
    SOCSYS_CLK_CTRL&=~CLK_CTRL_BCLK_WORK_SEL;  //SoC -> BPLL
    clkWaitBitStable(CLK_CTRL_CLK_RDY);        //confirm clock-switch ready

    SOCSYS_REG2C=SOCSYS_REG2C_CFG;             //enable peripheral clock gates
}

void IRQbspInit()
{
    //LEDs (PTB0/PTB1) as outputs, off; keep the power self-latch (PTB13) held.
    HD2_GPIOB_DDR|=(HD2_LED_GREEN|HD2_LED_RED|HD2_PWR_HOLD);
    HD2_GPIOB_DR =(HD2_GPIOB_DR & ~(HD2_LED_GREEN|HD2_LED_RED)) | HD2_PWR_HOLD;

    //Hard-lock forensics (hd2_crumb.h): snapshot the previous life's
    //breadcrumb block BEFORE IRQosTimerInit starts the new life's ISR stamps.
    //SRAM survives a watchdog reset, so after a WDT auto-recovery this holds
    //each context's last activity before the lock (diag op 'H' dumps it).
    {
        hd2_crumb_t *crumb = (hd2_crumb_t *)HD2_CRUMB_BASE;
        hd2_crumb_prev = *crumb;
        hd2_crumb_t fresh = {};
        fresh.magic = HD2_CRUMB_MAGIC;
        fresh.boot_count = 1u + (hd2_crumb_prev.magic == HD2_CRUMB_MAGIC ?
                                 hd2_crumb_prev.boot_count : 0u);
        *crumb = fresh;
    }

    //Bring up the polled-TX UART0 debug console (57600 8N1). Must run after
    //IRQmemoryAndClockInit (clk_init_pll) so the divisor matches the 42 MHz ref.
    hd2_dbg_init();
}

void bspInit2()
{
    //Nothing yet (no filesystem). bspInit2 runs after the kernel is up.
}

void shutdown()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    FastGlobalIrqLock::lock();
    //Cut the power self-latch (PTB13 low) — knob-off behaviour
    //(project_hd2_power_shutdown). If power is externally held, fall back to
    //rebooting so we don't spin forever powered.
    HD2_GPIOB_DR&=~HD2_PWR_HOLD;
    for(volatile unsigned int i=0;i<2000000u;++i) { }
    IRQsystemReboot();
}

void reboot()
{
    ioctl(STDOUT_FILENO,IOCTL_SYNC,0);
    FastGlobalIrqLock::lock();
    IRQsystemReboot();
}

} //namespace miosix
