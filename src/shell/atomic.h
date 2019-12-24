/*
 *  Shell library
 *  Atomic variables
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.03.2013 14:01:37
 *      Initial revision.
 *
 *	Revision 1.1, 05.04.2015 22:22:55
 *		Changed type atomic_t.value from int to int32_t.
 *
 */

#ifndef __SHELL_ATOMIC_H_INCLUDED__
#define __SHELL_ATOMIC_H_INCLUDED__

#include "shell/types.h"


typedef struct atomic  {
	volatile int32_t	value;
} atomic_t;

#define ZERO_ATOMIC {0}

#define atomic_set(__atomic, __value)		((__atomic)->value = (__value))
#define atomic_get(__atomic)				(__atomic)->value

#define atomic_add(__atomic, __value)	\
	__sync_add_and_fetch(&(__atomic)->value, (__value))

#define atomic_sub(__atomic, __value)	\
	__sync_sub_and_fetch(&(__atomic)->value, (__value))

#define atomic_inc(__atomic)				atomic_add(__atomic, 1)
#define atomic_dec(__atomic)				atomic_sub(__atomic, 1)

/*
 * Compare and swap
 *
 * If the current value of v is oldval,
 * then write newval into @b v. Returns TRUE if
 * the comparison is successful and newval was
 * written.
 */
#define atomic_cas(__atomic, __oldval, __newval)	\
	__sync_bool_compare_and_swap(&(__atomic)->value, __oldval, __newval)


#endif /* __SHELL_ATOMIC_H_INCLUDED__ */
