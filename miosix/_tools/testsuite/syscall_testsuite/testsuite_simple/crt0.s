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
 * \param v exit value 
 */
.section .text._exit
.global _exit
.type _exit, %function
_exit:
	movs r3, #2
	svc  0

/**
 * open, open a file
 * \param fd file descriptor
 * \param file access mode
 * \param xxx access permisions
 * \return file descriptor or -1 if errors
 */
.section .text.open
.global open
.type open, %function
open:
	movs r3, #6
	svc 0
	bx lr

/**
 * close, close a file
 * \param fd file descriptor
 */
.section .text.close
.global close
.type close, %function
close:
	movs r3, #7
	svc 0
	bx lr

/**
 * lseek
 * \param fd file descriptor
 * \param pos moving offset
 * \param start position, SEEK_SET, SEEK_CUR or SEEK_END
*/ 
.section .text.seek
.global seek
.type seek, %function
	
	movs r3, #8
	svc 0
	bx lr


/**
 * system, fork and execture a program, blocking
 * \param program to execute
 */
.section .text.system
.global system
.type system, %function
system:
	movs r3, #9
	svc 0
	bx lr
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
    movs r3, #3
    svc  0
    bx   lr

/**
 * read, read from file
 * \param fd file descriptor
 * \param buf data to be read
 * \param len buffer length
 * \return number of read bytes or -1 if errors
 */
.section .text.read
.global	read
.type	read, %function
read:
    movs r3, #4
    svc  0
    bx   lr

/**
 * usleep, sleep a specified number of microseconds
 * \param us number of microseconds to sleep
 * \return 0 on success or -1 if errors
 */
.section .text.usleep
.global	usleep
.type	usleep, %function
usleep:
    movs r3, #5
    svc  0
    bx   lr

.end
