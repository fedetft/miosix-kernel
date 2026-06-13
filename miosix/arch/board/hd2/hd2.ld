/*
 * Linker script for the Ailunce HD2 (HR_C7000 / CK803S).
 * Compiles Miosix as a unikernel: all RAM for the kernel + kernelspace app,
 * no process pool.
 *
 * Memory map (project_hd2_diagboot_works / project_hd2_vma_offset_bug):
 *   flash  the app slot the Dahua IAP loads + jumps to. VMA == LMA (we run
 *          straight from the slot, no remap). Base 0x0300d000, length 614400
 *          (600*1024) so the padded .bin slots into the same YMODEM envelope
 *          (scripts/fw_crypto.py); see feedback_hd2_flash_pad_size.
 *   ram    on-chip SRAM. We take 0x20000..0x50000 (192 KiB), leaving the low
 *          IRAM (0x10000..0x20000, the IAP/BOOTROM VBR + early use) untouched.
 *
 * The generic Miosix script defines all sections/symbols (_etext,_data,
 * _bss_start/_end, init arrays, ctors, _heap_end, _irq_stack_top, and
 * ENTRY(miosix::Reset_Handler), which our cpu interrupts.cpp places first in
 * flash via .isr_vector). The board only specifies MEMORY + tunables.
 */

/* IRQ stack: CK803S has a single SP; interrupts nest on it (CTX_SAVE pushes
 * the 72-byte frame). This stack is used during early boot and would also back
 * any deep nesting before the first context switch — keep it comfortable. */
_irq_stack_size = 1024;

MEMORY
{
    flash(rx) : ORIGIN = 0x0300d000, LENGTH = 614400
    /* Top 4 KiB (0x4f000..0x50000) excluded: the hard-lock forensics
     * breadcrumb block (openrtx_hd2/hd2_crumb.h) lives at 0x4f000 -- it must
     * survive a watchdog reset, so it sits outside .bss/heap AND below the
     * region the IAP scribbles on every boot (~0x4fe00..0x50000, measured
     * 2026-06-13 via a signature ladder across a real WDT crash cycle). */
    ram(wx)   : ORIGIN = 0x00020000, LENGTH = 0x2F000   /* 192 KiB - 4 KiB */
}

INCLUDE ldscripts/miosix-flash-ram.ld
