/*
 *  Shell library
 *  Low Memory allocation functions
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 10.08.2016 10:31:00
 *      Initial revision.
 *
 */

#ifndef __SHELL_MEMORY_H_INCLUDED__
#define __SHELL_MEMORY_H_INCLUDED__

#include <malloc.h>

#define memAlloc		::malloc
#define memFree			::free
#define memRealloc		::realloc

#define memAlloca		::alloca

#define SAFE_FREE(__p)	\
	do{ if ( (__p) != 0 ) memFree(__p);  (__p) = 0; }while(0)

#endif /* __SHELL_MEMORY_H_INCLUDED__ */
