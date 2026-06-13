/***************************************************************************
 *   Copyright (C) 2012-2025 by Terraneo Federico                          *
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
/* no .cpu specified, this file supports both thumb-1 and thumb-2 CPUs */
.thumb

/**
 * _start, program entry point
 * Function is not performance-critical as it is run only once to start the
 * process so it is written entirely using thumb-1 instructions to be compatible
 * with all ARM CPUs
 */
.section .text
.global _start
.type _start, %function
_start:
	/* Kernel calls here with r0=argc, r1=argv, r2=envp, r3=heapEnd, we save  */
	/* only argc, argv and envp for later. r3 is written to __processHeapEnd  */
	/* before we lose it calling constructors. Saving 3*4=12byte + 4bytes in  */
	/* __call_ctor respects 8 byte stack alignment in the called C++ ctors    */
	/* while __call_ctor itself uses nonstandard calling conventions anyway   */
	push  {r0,r1,r2}
	/* store the stack end (for profiling purposes) */
	mov   r7, r9        /* thumb-1 has limited access to upper registers */
	mov   r6, sp        /* thumb-1 has limited access to upper registers */
	ldr.n r0, .L100+0   /* __processStackEnd */
	ldr   r0, [r7, r0]
	str   r6, [r0]
	/* store the heap end in the appropriate variable */
	ldr.n r0, .L100+4   /* __processHeapEnd */
	ldr   r0, [r7, r0]
	str   r3, [r0]
	/* store envp in the appropriate variable */
	ldr.n r0, .L100+8   /* environ */
	ldr   r0, [r7, r0]
	str   r2, [r0]
	/* call C++ global constructors */
	ldr.n r0, .L100+12  /* __preinit_array_start */
	ldr.n r1, .L100+16  /* __preinit_array_end */
	ldr   r4, [r7, r0]
	ldr   r5, [r7, r1]
	bl    __call_ctor
	ldr.n r0, .L100+20  /* __init_array_start */
	ldr.n r1, .L100+24  /* __init_array_end */
	ldr   r4, [r7, r0]
	ldr   r5, [r7, r1]
	bl    __call_ctor
	pop   {r0,r1,r2}    /* get back argc,argv,envp to call main */
	bl    main
	/* Terminate the program, simply by calling exit().
     * Note: a previous version of _start manually called __call_exitprocs()
     * followed by _exit(), but it forgot to also call __stdio_exit_handler(),
     * which caused stdout to not be flushed on exit.
     * The standard exit() function does everything that needs to be done,
     * and is perfectly fine to call here, so we might as well use it!
     * Switched to bl insted of b to fix linker errors if the symbol is far */
	bl    exit
.align 2
.L100:
	.word	__processStackEnd(GOT)
	.word	__processHeapEnd(GOT)
	.word	environ(GOT)
	.word	__preinit_array_start(GOT)
	.word	__preinit_array_end(GOT)
	.word	__init_array_start(GOT)
	.word	__init_array_end(GOT)

/**
 * Call global constructors and destructors
 * Function is written entirely using thumb-1 instructions to be compatible with
 * all ARM CPUs
 * Uses non-standard calling convention, as it is called only from _start
 * expects the pointer to the start of the function pointer area in r4 and
 * the pointer to the end of it in r5
 */
.section .text
.type	__call_ctor, %function
__call_ctor:
	push {lr}
	cmp  r4, r5
	beq  .L201
.L200:
	ldmia r4!, {r3}     /* thumb-1 equivalent of ldr  r3, [r4], #4 */
	blx  r3
	cmp  r5, r4
	bne  .L200
.L201:
	pop  {pc}

/**
 * pthread_yield
 * At least on newlib, this function does not return a value
 */
.section .text.pthread_yield
.global pthread_yield
.type pthread_yield, %function
pthread_yield:
	push {r7,lr}
	movs r7, #0
	svc  0
	pop  {r7,pc}

/**
 * sched_yield
 * \return 0 (success)
 */
.section .text.sched_yield
.global sched_yield
.type sched_yield, %function
sched_yield:
	push {r7,lr}
	movs r7, #0
	svc  0
	movs r0, #0
	pop  {r7,pc}

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
	push {r7,lr}
	movs r7, #2
	svc  0
	cmp  r0, #0
	blt  .L300
	pop  {r7,pc}
.L300:
	b    syscallfailed32

/**
 * close, close a file
 * \param fd file descriptor
 * \return 0 on success, -1 on failure
 */
.section .text.close
.global close
.type close, %function
close:
	push {r7,lr}
	movs r7, #3
	svc  0
	cmp  r0, #0
	blt  .L400
	pop  {r7,pc}
.L400:
	b    syscallfailed32

/**
 * read, read from file
 * \param fd file descriptor
 * \param buf data to be read
 * \param size buffer length
 * \return number of read bytes or -1 if errors
 */
.section .text.read
.global	read
.type	read, %function
read:
	push {r7,lr}
	movs r7, #4
	svc  0
	cmp  r0, #0
	blt  .L500
	pop  {r7,pc}
.L500:
	b    syscallfailed32

/**
 * write, write to file
 * \param fd file descriptor
 * \param buf data to be written
 * \param size buffer length
 * \return number of written bytes or -1 if errors
 */
.section .text.write
.global	write
.type	write, %function
write:
	push {r7,lr}
	movs r7, #5
	svc  0
	cmp  r0, #0
	blt  .L600
	pop  {r7,pc}
.L600:
	b    syscallfailed32

/**
 * lseek
 * \param fd file descriptor, passed in r0
 * \param pos moving offset, passed in r3,r2 as it is a long long
 * \param whence, SEEK_SET, SEEK_CUR or SEEK_END, passed in the stack
 * \return absolute position after seek on success, -1LL on failure
 */
.section .text.lseek
.global lseek
.type lseek, %function
lseek:
	push {r7,lr}
	ldr  r1, [sp, #8]  /* Whence moved to 2nd syscall parameter (r1) */
	movs r7, #6
	svc  0
	cmp  r1, #0        /* 64 bit negative check */
	blt  .L700
	pop  {r7,pc}
.L700:
	b    syscallfailed64

/**
 * stat
 * \param path path to file or directory
 * \param pstat pointer to struct stat
 * \return 0 on success, -1 on failure
 */
.section .text.stat
.global stat
.type stat, %function
stat:
	push {r7,lr}
	movs r7, #7
	svc  0
	cmp  r0, #0
	blt  .L800
	pop  {r7,pc}
.L800:
	b    syscallfailed32

/**
 * lstat
 * \param path path to file or directory
 * \param pstat pointer to struct stat
 * \return 0 on success, -1 on failure
 */
.section .text.lstat
.global lstat
.type lstat, %function
lstat:
	push {r7,lr}
	movs r7, #8
	svc  0
	cmp  r0, #0
	blt  .L900
	pop  {r7,pc}
.L900:
	b    syscallfailed32

/**
 * fstat
 * \param fd file descriptor
 * \param pstat pointer to struct stat
 * \return 0 on success, -1 on failure
 */
.section .text.fstat
.global fstat
.type fstat, %function
fstat:
	push {r7,lr}
	movs r7, #9
	svc  0
	cmp  r0, #0
	blt  .L1000
	pop  {r7,pc}
.L1000:
	b    syscallfailed32

/**
 * fcntl
 * \param fd file descriptor
 * \param cmd operation to perform
 * \param opt optional third parameter
 * \return -1 on failure, an operation dependent return value otherwise
 */
.section .text.fcntl
.global fcntl
.type fcntl, %function
fcntl:
	push {r7,lr}
	movs r7, #10
	svc  0
	cmp  r0, #0
	blt  .L1100
	pop  {r7,pc}
.L1100:
	b    syscallfailed32

/**
 * ioctl
 * \param fd file descriptor
 * \param cmd operation to perform
 * \param arg optional third parameter
 * \return -1 on failure, an operation dependent return value otherwise
 */
.section .text.ioctl
.global ioctl
.type ioctl, %function
ioctl:
	push {r7,lr}
	movs r7, #11
	svc  0
	cmp  r0, #0
	blt  .L1200
	pop  {r7,pc}
.L1200:
	b    syscallfailed32

/**
 * isatty
 * \param fd file descriptor
 * \return 1 if fd is associated with a terminal, 0 if not, or also on failure.
 * When fd is valid but not a terminal, errno is set to ENOTTY.
 */
.section .text.isatty
.global isatty
.type isatty, %function
isatty:
	push {r7,lr}
	movs r7, #12
	svc  0
	cmp  r0, #0
	bgt  .L1300
	bl   __isattyfailed
.L1300:
	pop  {r7,pc}

/**
 * getcwd
 * \param fd file descriptor
 * \param buf pointer to buffer where current directory will be written
 * \param size buffer size
 * \return pointer on success, NULL on failure
 */
.section .text.getcwd
.global getcwd
.type getcwd, %function
getcwd:
	push {r7,lr}
	movs r7, #13
	svc  0
	cmp  r1, #0
	bge  .L1400
	movs r0, r1
	bl   __getcwdfailed
.L1400:
	pop  {r7,pc}

/**
 * chdir
 * \param path pointer to path where to change directory
 * \return 0 on success, -1 on failure
 */
.section .text.chdir
.global chdir
.type chdir, %function
chdir:
	push {r7,lr}
	movs r7, #14
	svc  0
	cmp  r0, #0
	blt  .L1500
	pop  {r7,pc}
.L1500:
	b    syscallfailed32

/**
 * getdents
 * \param fd file descriptor
 * \param buf pointer to buffer where directory entries will be written
 * \param size buffer size
 * \return number of bytes wrtten, 0 on end of directory, -1 on failure
 */
.section .text.getdents
.global getdents
.type getdents, %function
getdents:
	push {r7,lr}
	movs r7, #15
	svc  0
	cmp  r0, #0
	blt  .L1600
	pop  {r7,pc}
.L1600:
	b    syscallfailed32

/**
 * mkdir
 * \param path pointer to path of directory to create
 * \param mode directory mode
 * \return 0 on success, -1 on failure
 */
.section .text.mkdir
.global mkdir
.type mkdir, %function
mkdir:
	push {r7,lr}
	movs r7, #16
	svc  0
	cmp  r0, #0
	blt  .L1700
	pop  {r7,pc}
.L1700:
	b    syscallfailed32

/**
 * rmdir
 * \param path pointer to path of directory to remove
 * \return 0 on success, -1 on failure
 */
.section .text.rmdir
.global rmdir
.type rmdir, %function
rmdir:
	push {r7,lr}
	movs r7, #17
	svc  0
	cmp  r0, #0
	blt  .L1800
	pop  {r7,pc}
.L1800:
	b    syscallfailed32

/**
 * link
 * \param oldpath existing file path
 * \param newpath new file path
 * \return 0 on success, -1 on failure
 */
.section .text.link
.global link
.type link, %function
link:
	push {r7,lr}
	movs r7, #18
	svc  0
	cmp  r0, #0
	blt  .L1900
	pop  {r7,pc}
.L1900:
	b    syscallfailed32

/**
 * unlink
 * \param path path of file/directory to remove
 * \return 0 on success, -1 on failure
 */
.section .text.unlink
.global unlink
.type unlink, %function
unlink:
	push {r7,lr}
	movs r7, #19
	svc  0
	cmp  r0, #0
	blt  .L2000
	pop  {r7,pc}
.L2000:
	b    syscallfailed32

/**
 * symlink
 * \param target symlink destination
 * \param linkpath file name of symlink to be created
 * \return 0 on success, -1 on failure
 */
.section .text.symlink
.global symlink
.type symlink, %function
symlink:
	push {r7,lr}
	movs r7, #20
	svc  0
	cmp  r0, #0
	blt  .L2100
	pop  {r7,pc}
.L2100:
	b    syscallfailed32

/**
 * readlink
 * \param path path to the symlink
 * \param buf pointer where the symlink target will be stored
 * \param size buffer size
 * \return 0 on success, -1 on failure
 */
.section .text.readlink
.global readlink
.type readlink, %function
readlink:
	push {r7,lr}
	movs r7, #21
	svc  0
	cmp  r0, #0
	blt  .L2200
	pop  {r7,pc}
.L2200:
	b    syscallfailed32

/**
 * truncate
 * \param path path to the file
 * \param size new file size
 * \return 0 on success, -1 on failure
 */
.section .text.truncate
.global truncate
.type truncate, %function
truncate:
	push {r7,lr}
	movs r7, #22
	svc  0
	cmp  r0, #0
	blt  .L2300
	pop  {r7,pc}
.L2300:
	b    syscallfailed32

/**
 * ftruncate
 * \param fd file descriptor
 * \param size new file size
 * \return 0 on success, -1 on failure
 */
.section .text.ftruncate
.global ftruncate
.type ftruncate, %function
ftruncate:
	push {r7,lr}
	movs r7, #23
	svc  0
	cmp  r0, #0
	blt  .L2400
	pop  {r7,pc}
.L2400:
	b    syscallfailed32

/**
 * rename
 * \param oldpath existing file path
 * \param newpath new file path
 * \return 0 on success, -1 on failure
 */
.section .text.rename
.global rename
.type rename, %function
rename:
	push {r7,lr}
	movs r7, #24
	svc  0
	cmp  r0, #0
	blt  .L2500
	pop  {r7,pc}
.L2500:
	b    syscallfailed32

/* TODO: missing syscalls: chmod, fchmod, chown, fchown, lchown */

/**
 * dup
 * \param fd file descriptor to duplicate
 * \return the new file descriptor on success, -1 on failure
 */
.section .text.dup
.global dup
.type dup, %function
dup:
	push {r7,lr}
	movs r7, #30
	svc  0
	cmp  r0, #0
	blt  .L3100
	pop  {r7,pc}
.L3100:
	b    syscallfailed32

/**
 * dup2
 * \param oldfd file descriptor to duplicate
 * \param newfd old file descriptor will be duplicated to this file descriptor
 * \return the new file descriptor on success, -1 on failure
 */
.section .text.dup2
.global dup2
.type dup2, %function
dup2:
	push {r7,lr}
	movs r7, #31
	svc  0
	cmp  r0, #0
	blt  .L3200
	pop  {r7,pc}
.L3200:
	b    syscallfailed32

/**
 * pipe
 * \param fds[2] file descriptors
 * \return 0 on success, -1 on failure
 */
.section .text.pipe
.global pipe
.type pipe, %function
pipe:
	push {r7,lr}
	movs r3, r0        /* save fds to a non overwritten register */
	movs r7, #32
	svc  0
	cmp  r0, #0
	blt  .L3300
	str  r1, [r3]
	str  r2, [r3, #4]
	pop  {r7,pc}
.L3300:
	b    syscallfailed32

/* TODO: missing syscalls: access */

/**
 * miosix::getTime, nonstandard syscall
 * \return long long time in nanoseconds, relative to clock monotonic
 */
.section .text._ZN6miosix7getTimeEv
.global _ZN6miosix7getTimeEv
.type _ZN6miosix7getTimeEv, %function
_ZN6miosix7getTimeEv:
	push {r7,lr}
	movs r7, #36
	svc  0
	pop  {r7,pc}

/**
 * miosix::nanoSleepUntil, nonstandard syscall
 * \param absolute sleep time in nanoseconds, relative to clock monotonic
 */
.section .text._ZN6miosix14nanoSleepUntilEx
.global _ZN6miosix14nanoSleepUntilEx
.type _ZN6miosix14nanoSleepUntilEx, %function
_ZN6miosix14nanoSleepUntilEx:
	push {r7,lr}
	movs r7, #37
	svc  0
	pop  {r7,pc}

/**
 * clock_gettime
 * \param clockid which clock
 * \param tp struct timespec*
 * \return in Miosix this syscall always returns 0, if the clockid is wrong
 * the default clock is returned
 */
.section .text.clock_gettime
.global clock_gettime
.type clock_gettime, %function
clock_gettime:
	push {r4, r7}
	movs r4, r1
	movs r7, #38
	svc  0
#if __ARM_ARCH_ISA_THUMB == 1
	str  r2, [r4]
	str  r3, [r4, #4]
#else
	strd r2, [r4]
#endif
	str  r1, [r4, #8]
	pop  {r4, r7}
	cmp  r0, #0
	blt  .L3900        /* can't use syscallfailed32 as we haven't pushed r7,lr */
	bx   lr
.L3900:
	b    __seterrno32 @ tail call

/**
 * clock_settime
 * \param clockid which clock
 * \param tp struct timespec*
 * \return 0 on success or a positive error code
 */
.section .text.clock_settime
.global clock_settime
.type clock_settime, %function
clock_settime:
	push {r7, lr}
	/* return syscall(clock_id,tp->tv_nsec,tp->tv_sec); */
#if __ARM_ARCH_ISA_THUMB == 1
	ldr  r2, [r1]
	ldr  r3, [r1, #4]
#else
	ldrd r2, [r1]
#endif
	ldr  r1, [r1, #8]
	movs r7, #39
	svc  0
	cmp  r0, #0
	blt  .L4000
	pop  {r7, pc}
.L4000:
	b    syscallfailed32

/**
 * clock_nanosleep
 * \param clockid which clock
 * \param flags absolute or relative
 * \param req struct timespec* with sleep time
 * \param rem optional struct timespec*
 * \return 0 on success or a positive error code
 */
.section .text.clock_nanosleep
.global clock_nanosleep
.type clock_nanosleep, %function
clock_nanosleep:
	push {r7, lr}
	movs r7, #40
	svc  0
	pop  {r7,pc}

/**
 * clock_getres
 * \param clockid which clock
 * \param req struct timespec* resolution
 * \return 0 on success or a positive error code
 */
.section .text.clock_getres
.global clock_getres
.type clock_getres, %function
clock_getres:
	push {r7,lr}
	movs r2, r1       /* save req to a non overwritten register */
	movs r7, #41
	svc  0
	cmp  r2, #0
	beq  .L4200        /* req can be nullptr */
	str  r1, [r2, #8]
	movs r3, #0
	str  r3, [r2, #4]
	str  r3, [r2, #0]
.L4200:
	cmp  r0, #0
	blt  .L4201
	pop  {r7,pc}
.L4201:
	b    syscallfailed32

/* TODO: missing syscalls: clock_adjtime */

/**
 * _exit, terminate process
 * \param v exit value
 * This syscall does not return
 */
.section .text._exit
.global _exit
.type _exit, %function
_exit:
	movs r7, #43
	svc  0

/**
 * _execve, run a different program
 * \param path program to run
 * \param argv program arguments
 * \param envp program environment variables
 * \return -1 on failure. Does not return on success
 */
.section .text._execve
.global _execve
.type _execve, %function
_execve:
	push {r7,lr}
	movs r7, #44
	svc  0
	b    syscallfailed32 /* if execve returns, then it failed */

/**
 * __do_spawn, syscall underneath posix_spawn, start a new process running the
 * given program
 * \param ptr pointer to a struct SpawnArgs with the following fields
 * - path program to run
 * - file_actions optional actions
 * - attrp more options
 * - argv program arguments
 * - envp program environment variables
 * \return the pid of the spawned process or a negative error code on failure
 */
.section .text.__do_spawn
.global __do_spawn
.type __do_spawn, %function
__do_spawn:
	push {r7,lr}
	movs r7, #45
	svc  0
	pop  {r7,pc}

/* TODO: missing syscalls */

/**
 * waitpid, wait for process termination
 * \param pid pid to wait for
 * \param wstatus pointer to return code
 * \param options wait options
 * \return 0 on success, -1 on failure
 */
.section .text.waitpid
.global waitpid
.type waitpid, %function
waitpid:
	push {r7,lr}
	movs r7, #47
	svc  0
	cmp  r0, #0
	blt  .L4800
	pop  {r7,pc}
.L4800:
	b    syscallfailed32

/**
 * getpid
 * \return the pid of the current process
 */
.section .text.getpid
.global getpid
.type getpid, %function
getpid:
	push {r7,lr}
	movs r7, #48
	svc  0
	pop  {r7,pc}

/**
 * getppid
 * \return the pid of the parent process
 */
.section .text.getppid
.global getppid
.type getppid, %function
getppid:
	push {r7,lr}
	movs r7, #49
	svc  0
	pop  {r7,pc}

/* TODO: missing syscalls: getuid, getgid, geteuid, getegid, setuid, setgid */

/**
 * sysconf
 * \param query requested value
 * \return desired value on success, -1 on failure
 */
.section .text.sysconf
.global sysconf
.type sysconf, %function
sysconf:
	push {r7,lr}
	movs r7, #59
	svc  0
	cmp  r0, #0
	blt  .L6000
	pop  {r7,pc}
.L6000:
	b    syscallfailed32

/* common jump target for all failing syscalls with 32 bit return value */
.section .text.__seterrno32
syscallfailed32:
#if __ARM_ARCH_ISA_THUMB == 1
	ldr  r7, [sp, #4] /* push{r7,lr} leaves lr @ sp+4 and r7 @ sp+0 */
	mov  lr, r7
	ldr  r7, [sp, #0]
	add  sp, #8
#else
	pop {r7,lr}
#endif
	b    __seterrno32 @ tail call

/* common jump target for all failing syscalls with 64 bit return value */
.section .text.__seterrno64
syscallfailed64:
#if __ARM_ARCH_ISA_THUMB == 1
	ldr  r7, [sp, #4] /* push{r7,lr} leaves lr @ sp+4 and r7 @ sp+0 */
	mov  lr, r7
	ldr  r7, [sp, #0]
	add  sp, #8
#else
	pop {r7,lr}
#endif
	b    __seterrno64 @ tail call

.end
