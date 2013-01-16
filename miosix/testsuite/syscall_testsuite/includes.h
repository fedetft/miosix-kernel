#ifndef _APP_SYSCALL_TESTS_
#define _APP_SYSCALL_TESTS_
#include "miosix/testsuite/syscall_testsuite/testsuite_syscall.h"
	#include "miosix/testsuite/syscall_testsuite/testsuite_simple.h"
	#include "miosix/testsuite/syscall_testsuite/testsuite_sleep.h"
	#include "miosix/testsuite/syscall_testsuite/testsuite_system.h"

	#ifdef WITH_FILESYSTEM
		#include "miosix/testsuite/syscall_testsuite/testsuite_file1.h"
		#include "miosix/testsuite/syscall_testsuite/testsuite_file2.h"
		#include "miosix/testsuite/syscall_testsuite/testsuite_syscall_mpu_open.h"
		#include "miosix/testsuite/syscall_testsuite/testsuite_syscall_mpu_read.h"
		#include "miosix/testsuite/syscall_testsuite/testsuite_syscall_mpu_write.h"
	#endif
#endif //_APP_SYSCALL_TESTS_
