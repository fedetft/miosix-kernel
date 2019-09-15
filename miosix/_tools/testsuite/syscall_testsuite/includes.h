#ifndef _APP_SYSCALL_TESTS_
#define _APP_SYSCALL_TESTS_
#include "testsuite_syscall.h"
#include "testsuite_simple.h"
#include "testsuite_sleep.h"
#include "testsuite_system.h"

#ifdef WITH_FILESYSTEM
#include "testsuite_file1.h"
#include "testsuite_file2.h"
#include "testsuite_syscall_mpu_open.h"
#include "testsuite_syscall_mpu_read.h"
#include "testsuite_syscall_mpu_write.h"
#endif
#endif //_APP_SYSCALL_TESTS_
