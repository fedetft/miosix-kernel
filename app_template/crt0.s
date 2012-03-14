/*
 * Startup script for writing PROGRAMS for the Miosix embedded OS
 * TFT:Terraneo Federico Technlogies
 */

.syntax unified
.cpu cortex-m3
.thumb

.section .text

/**
 * _start, program entry point
 */
.global _start
.type _start, %function
_start:
	/* TODO: .ctor */
	bl   main
	/* TODO: .dtor */
	bl   _exit

/**
 * _exit, terminate process
 * \param r0 exit value 
 */
.section .text._exit
.global _exit
.type _exit, %function
_exit:
	movs r3, #1
	svc  0

/**
 * write, write to file
 * \param fd file descriptor
 * \param buf data to be written
 * \param len buffer length
 * \return number of written bytes or -1 if errors
 */
.section .text.write
.global	write
.type	write, %function
write:
    movs r3, #2
    svc  0
    bx   lr

.end
