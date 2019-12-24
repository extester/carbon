/*
 *  Carbon framework
 *  Custom c++ new/delete for various classes (STM32)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 15.02.2017 12:02:12
 *      Initial revision.
 */

#include "shell/config.h"
#include "shell/static_allocator.h"

#include "carbon/timer.h"
#include "carbon/event.h"
#include "carbon/logger.h"

/*******************************************************************************
 * CTimer class static memory allocator
 */

#define TIMER_ALLOC_SIZE	ALIGN(sizeof(CTimer), 4)

static uint8_t timers[CARBON_TIMER_COUNT*TIMER_ALLOC_SIZE];

static CStaticAllocator<TIMER_ALLOC_SIZE, 4>	timerAllocator(timers, sizeof(timers));


void* CTimer::operator new(size_t size) throw()
{
	void*	pData;

	//log_debug(L_GEN, "----------------- S_ALLOC TIMER ---------------\n");

	shell_assert(size <= TIMER_ALLOC_SIZE);
	pData = timerAllocator.alloc();
	if ( !pData ) {
		no_static_memory();
	}
	return pData;
}

void* CTimer::operator new[](size_t size) throw()
{
	UNUSED(size);

	shell_assert_ex(FALSE, "array of timers allocation is not supported\n");
	no_static_memory();
	return 0;
}

void CTimer::operator delete(void* pData)
{
	//log_debug(L_GEN, "===================== S_FREE TIMER ======================\n");

	if ( pData ) {
		timerAllocator.dealloc(pData);
	}
}

void CTimer::operator delete[](void* pData)
{
	UNUSED(pData);

	shell_assert_ex(FALSE, "array of timers deallocation is not supported\n");
	no_static_memory();
}

/*******************************************************************************
 * CEvent class static memory allocator
 */

#define EVENT_ALLOC_SIZE	ALIGN(sizeof(CTimer), 4)

static uint8_t events[CARBON_EVENT_COUNT*EVENT_ALLOC_SIZE];

static CStaticAllocator<EVENT_ALLOC_SIZE, 4>	eventAllocator(events, sizeof(events));


void* CEvent::operator new(size_t size) throw()
{
	void*	pData;

	//log_debug(L_GEN, "----------------- S_ALLOC EVENT ---------------\n");

	shell_assert(size <= EVENT_ALLOC_SIZE);
	pData = eventAllocator.alloc();
	if ( !pData )  {
		no_static_memory();
	}
	return pData;
}

void* CEvent::operator new[](size_t size) throw()
{
	UNUSED(size);

	shell_assert_ex(FALSE, "array of events allocation is not supported\n");
	no_static_memory();
	return 0;
}

void CEvent::operator delete(void* pData)
{
	//log_debug(L_GEN, "===================== S_FREE EVENT ======================\n");

	if ( pData ) {
		eventAllocator.dealloc(pData);
	}
}

void CEvent::operator delete[](void* pData)
{
	UNUSED(pData);

	shell_assert_ex(FALSE, "array of events deallocation is not supported\n");
	no_static_memory();
}
