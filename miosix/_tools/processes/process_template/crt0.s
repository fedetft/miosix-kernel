/***************************************************************************
 *   Copyright (C) 2012-2020 by Terraneo Federico                          *
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

.syntax unified
.arch armv7-m
.thumb

/**
 * call global constructors and destructors
 * uses non-standard calling convention, as it is called only from _start
 * expects the pointer to the start of the function pointer area in r4 and
 * the pointer to the end of it in r5
 */
.section .text
.type	call, %function
call:
	push {lr}
	cmp  r4, r5
	beq  .L100
.L101:
	ldr  r3, [r4], #4
	blx  r3
	cmp  r5, r4
	bne  .L101
.L100:
	pop  {pc}

/**
 * _start, program entry point
 */
.section .text
.global _start
.type _start, %function
_start:
	/* call C++ global constructors */
	ldr  r0, .L200
	ldr  r1, .L200+4
	ldr  r4, [r9, r0]
	ldr  r5, [r9, r1]
	bl   call
	ldr  r0, .L200+8
	ldr  r1, .L200+12
	ldr  r4, [r9, r0]
	ldr  r5, [r9, r1]
	bl   call
	/* call main */
	bl   main
	mov  r6, r0
	/* atexit */
	bl   __call_exitprocs
	/* for now we don't call function pointers in __fini_array as nobody seems
	   to use these. Even C++ global destructors are registered through atexit
	   and the implementation of exit() in newlib doesn't touch __fini_array
	   either, so doing it here would even be inconsistent with exit() */
	/* terminate the program */
	mov  r0, r6
	b    _exit
.L200:
	.word	__preinit_array_end(GOT)
	.word	__preinit_array_start(GOT)
	.word	__init_array_start(GOT)
	.word	__init_array_end(GOT)

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
	cmp  r0, #0
	blt  syscallfailed
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
	cmp  r0, #0
	blt  syscallfailed
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

/**
 * open, open a file
 * \param path file name
 * \param file access mode
 * \param mode access permisions
 * \return file descriptor or -1 if errors
 */
.section .text.open
.global open
.type open, %function
open:
	movs r3, #6
	svc  0
	cmp  r0, #0
	blt  syscallfailed
	bx   lr

/**
 * close, close a file
 * \param fd file descriptor
 */
.section .text.close
.global close
.type close, %function
close:
	movs r3, #7
	svc  0
	bx   lr

/**
 * lseek
 * \param fd file descriptor
 * \param pos moving offset
 * \param start position, SEEK_SET, SEEK_CUR or SEEK_END
 */
.section .text.lseek
.global lseek
.type lseek, %function
lseek:
	movs r3, #8
	svc  0
	cmp  r0, #0
	blt  syscallfailed
	bx   lr

/**
 * system, fork and execture a program, blocking
 * \param program to execute
 */
.section .text.system
.global system
.type system, %function
system:
	movs r3, #9
	svc  0
	cmp  r0, #0
	blt  syscallfailed
	bx   lr

.section .text.__seterrno
/* common jump target for all failing syscalls */
syscallfailed:
	/* tail call */
	b    __seterrno

.end
