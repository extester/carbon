/*
 *  Shell library
 *  Independent STDIO operations
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 10.01.2017, 11:57:05
 *      Initial revision.
 */

#ifndef __SHELL_TSTDIO_H_INCLUDED__
#define __SHELL_TSTDIO_H_INCLUDED__

#include <stdio.h>

#define _tvsnprintf		vsnprintf
#define _tsnprintf		snprintf
#define _tsscanf		sscanf

#if __stm32__

int	_EXFUN(snprintf, (char *__restrict, size_t, const char *__restrict, ...)
               _ATTRIBUTE ((__format__ (__printf__, 3, 4))));

int	_EXFUN(vsnprintf, (char *__restrict, size_t, const char *__restrict, __VALIST)
               _ATTRIBUTE ((__format__ (__printf__, 3, 0))));

#endif /* __stm32__ */

#endif /* __SHELL_TSTDIO_H_INCLUDED__ */
