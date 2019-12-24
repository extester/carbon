/*
 *  Shell library
 *  Custom STD allocator for functor objects (embed)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.04.2017 17:36:35
 *      Initial revision.
 */

#include "shell/config.h"
#include "shell/static_allocator.h"
#include "shell/embed/functor_allocator.h"

#define FUNCTOR_ALLOC_SIZE				12
#define FUNCTOR_HEAD_SIZE				(FUNCTOR_ALLOC_SIZE*(CARBON_TIMER_COUNT+CARBON_THREAD_COUNT)*2)

static uint8_t buffer[FUNCTOR_HEAD_SIZE];

static CStaticAllocator<FUNCTOR_ALLOC_SIZE, 4>
	functorAllocator(buffer, FUNCTOR_HEAD_SIZE);


extern void* operator new(size_t n);
extern void operator delete(void* p);

void* operator new(size_t n)
{
	void*	p;

	shell_assert(n == FUNCTOR_ALLOC_SIZE);
	if ( n != FUNCTOR_ALLOC_SIZE ) {
		log_debug(L_ALWAYS_ENABLED, " INVALID ALLOCATION SIZE %d \n", n);
		no_static_memory();
		return NULL;
	}

	p = functorAllocator.alloc();
	if ( p == 0 )  {
		no_static_memory();
	}

	return p;
}

void operator delete(void* p)
{
	if ( p )  {
		functorAllocator.dealloc(p);
	}
}
