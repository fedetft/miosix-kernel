/*
 *  Common routine to implement atexit-like functionality.
 */

#include <stddef.h>
#include <stdlib.h>
#include <reent.h>
#include <sys/lock.h>
#include "atexit.h"

/* Make this a weak reference to avoid pulling in malloc.  */
void * malloc(size_t) _ATTRIBUTE((__weak__));
__LOCK_INIT_RECURSIVE(, __atexit_lock);

/*
 * Register a function to be performed at exit or on shared library unload.
 */

int
_DEFUN (__register_exitproc,
	(type, fn, arg, d),
	int type _AND
	void (*fn) (void) _AND
	void *arg _AND
	void *d)
{
	// Miosix is designed for microcontrollers, and in microcontrollers
	// main() usually never returns. Therefore, there's no reason to
	// take up memory storing finalizers registered with atexit(), 
	// onexit() and cxa_atexit().
	
	// Return zero as-if we did something
  return 0;
}
