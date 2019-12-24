/*
 *  Shell library
 *  Statistic counters
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.03.2013 13:29:48
 *      Initial revision.
 */

#ifndef	__SHELL_COUNTER_H_INCLUDED__
#define	__SHELL_COUNTER_H_INCLUDED__

#include "shell/atomic.h"
#include "shell/tstring.h"

/*
 *	We can count various interesting events and paths.
 *
 *	Use counter() to change the counters, eg:
 *		counter(c_idle_thread_block++);
 *	Use counter_always() for non-conditional counters.
 */
typedef atomic_t	counter_t;

#define counter_add(__counter, __delta)     atomic_add(&(__counter), __delta)
#define counter_sub(__counter, __delta)     atomic_sub(&(__counter), __delta)
#define counter_inc(__counter)          	counter_add(__counter, 1)
#define counter_dec(__counter)          	counter_sub(__counter, 1)

#define counter_get(__counter)          	atomic_get(&(__counter))
#define counter_set(__counter, __count)		atomic_set(&(__counter), (__count))

#define counter_init(__counter)         	atomic_set(&(__counter), 0)
#define counter_reset_struct(__struct)		_tbzero_object(__struct)

#define ZERO_COUNTER    					ZERO_ATOMIC

#endif	/* __SHELL_COUNTER_H_INCLUDED__ */

