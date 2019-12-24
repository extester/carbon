/*
 *  Carbon framework
 *  Custom c++ new/delete for various classes (UNIX)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 * Note: Dynamic memory allocations
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 15.02.2017 11:10:16
 *      Initial revision.
 */

#include <new>

#include "carbon/timer.h"
#include "carbon/event.h"
#include "carbon/memory.h"
#include "carbon/logger.h"

static void* default_dynamic_alloc(size_t size) throw()
{
	void*	pData;

	pData = memAlloc(size);
	if ( pData )  {
		return pData;
	}

	throw std::bad_alloc();
	return NULL;
}

static void default_dymanic_free(void* pData)
{
	if ( pData ) {
		memFree(pData);
	}
}

/*******************************************************************************
 * CTimer class dynamic memory allocator
 */

void* CTimer::operator new(size_t size) throw()
{
	//log_debug(L_GEN, "----------------- D_ALLOC TIMER ---------------\n");
	return default_dynamic_alloc(size);
}

void* CTimer::operator new[](size_t size) throw()
{
	//log_debug(L_GEN, "----------------- D_ALLOC TIMER[] ---------------\n");
	return default_dynamic_alloc(size);
}

void CTimer::operator delete(void* pData)
{
	//log_debug(L_GEN, "===================== D_FREE TIMER ======================\n");
	default_dymanic_free(pData);
}

void CTimer::operator delete[](void* pData)
{
	//log_debug(L_GEN, "===================== D_FREE TIMER[] ======================\n");
	default_dymanic_free(pData);
}


/*******************************************************************************
 * CEvent class dynamic memory allocator
 */

void* CEvent::operator new(size_t size) throw()
{
	//log_debug(L_GEN, "----------------- D_ALLOC EVENT ---------------\n");
	return default_dynamic_alloc(size);
}

void* CEvent::operator new[](size_t size) throw()
{
	//log_debug(L_GEN, "----------------- D_ALLOC EVENT[] ---------------\n");
	return default_dynamic_alloc(size);
}

void CEvent::operator delete(void* pData)
{
	//log_debug(L_GEN, "===================== D_FREE EVENT ======================\n");
	default_dymanic_free(pData);
}

void CEvent::operator delete[](void* pData)
{
	//log_debug(L_GEN, "===================== D_FREE EVENT[] ======================\n");
	default_dymanic_free(pData);
}
