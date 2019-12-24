/*
 *  Carbon framework
 *  Memory allocation manager
 *
 *  Copyright (c) 2015-2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.04.2015 22:39:08
 *      Initial revision.
 *
 *  Revision 2.0, 13.07.2015 14:48:51
 *  	Completle rewrote to use jemalloc library.
 *
 *  Revision 3.0, 19.10.2018 17:37:36
 *  	Removed deprecated glibc memory hooks and
 *  	redefine weak malloc/free functions.
 */
/*
 * Note:
 * 		Can't use log functions in alloc()/free().
 */

#include "shell/config.h"
#if CARBON_JEMALLOC
#include <jemalloc/jemalloc.h>
#endif /* CARBON_JEMALLOC */

#include "carbon/logger.h"
#include "carbon/memory.h"

#define USE_DEPRECATED_HOOK		0

static memory_stat_t	g_stat = {
	ZERO_ATOMIC, ZERO_ATOMIC, ZERO_ATOMIC, ZERO_ATOMIC,
	ZERO_ATOMIC, ZERO_ATOMIC, ZERO_ATOMIC
};

/*******************************************************************************
 * Memory Allocation Manager
 */

void CMemoryManager::getStat(void* pBuffer, size_t nSize) const
{
	size_t rsize = MIN(nSize, sizeof(g_stat));

	UNALIGNED_MEMCPY(pBuffer, &g_stat, rsize);
}

void CMemoryManager::resetStat()
{
	atomic_set(&g_stat.full_alloc_count, 0);
	atomic_set(&g_stat.free_count, 0);
	atomic_set(&g_stat.alloc_count, 0);
	atomic_set(&g_stat.alloc_size, 0);
	atomic_set(&g_stat.alloc_size_max, 0);
	atomic_set(&g_stat.realloc_count, 0);
	atomic_set(&g_stat.fail_count, 0);
}

void CMemoryManager::dump(const char* strPref) const
{
	log_dump("%s*** MemoryManager statistic:\n", strPref);
	log_dump("     allocs:                  %u\n", atomic_get(&g_stat.full_alloc_count));
	log_dump("     freeds:                  %u\n", atomic_get(&g_stat.free_count));
	log_dump("     reallocs:                %u\n", atomic_get(&g_stat.realloc_count));
	log_dump("     failed:                  %u\n", atomic_get(&g_stat.fail_count));
	log_dump("     allocated:               %u\n", atomic_get(&g_stat.alloc_count));
	log_dump("     allocated size:          %u\n", atomic_get(&g_stat.alloc_size));
	log_dump("     max allocated size:      %u\n", atomic_get(&g_stat.alloc_size_max));
}

#if CARBON_JEMALLOC

/*******************************************************************************
 * Wrapper functions
 */

#if USE_DEPRECATED_HOOK

static void update_max_value(size_t nSize)
{
	size_t		nOldSize;
	int 		i = 20;

	do {
		nOldSize = (size_t)atomic_get(&g_stat.alloc_size_max);
	} while ( nOldSize < nSize &&
			  !atomic_cas(&g_stat.alloc_size_max, nOldSize, nSize)
			  && i-- > 0);
}

static void stat_alloc(size_t nSize)
{
	size_t nCurSize;

	atomic_inc(&g_stat.full_alloc_count);
	atomic_inc(&g_stat.alloc_count);
	nCurSize = atomic_add(&g_stat.alloc_size, nSize);
	update_max_value(nCurSize);
}

static void stat_free(size_t nSize)
{
	atomic_inc(&g_stat.free_count);
	atomic_dec(&g_stat.alloc_count);
	atomic_sub(&g_stat.alloc_size, nSize);
}

static void stat_realloc(ssize_t nDelta)
{
	size_t nCurSize;

	atomic_inc(&g_stat.realloc_count);
	nCurSize = atomic_add(&g_stat.alloc_size, nDelta);
	update_max_value(nCurSize);
}

static void stat_failed()
{
	atomic_inc(&g_stat.fail_count);
}

static void* wrapMallocHook(size_t nSize, const void* caller)
{
	void*	ptr;

	ptr = je_malloc(nSize);
	if ( ptr != NULL )  {
		stat_alloc(je_sallocx(ptr, 0));
	}
	else {
		if ( nSize > 0 ) {
			stat_failed();
		}
	}

	return ptr;
}

static void* wrapReallocHook(void* ptr, size_t nNewSize, const void* caller)
{
	void*	ptrNew;

	if ( ptr != NULL )  {
		ssize_t		nOldSize, nNewReal;

		nOldSize = je_sallocx(ptr, 0);

		ptrNew = je_realloc(ptr, nNewSize);
		if ( nNewSize > 0 ) {
			if ( ptrNew != NULL ) {
				nNewReal = je_sallocx(ptrNew, 0);
				stat_realloc(nNewReal - nOldSize);
			}
			else {
				stat_failed();
			}
		}
		else {
			stat_free((size_t)nOldSize);
		}
	}
	else {
		ptrNew = wrapMallocHook(nNewSize, caller);
	}

	return ptrNew;
}

static void wrapFreeHook(void* ptr, const void* caller)
{
	if ( ptr != NULL )  {
		size_t	nSize;

		nSize = je_sallocx(ptr, 0);

		je_free(ptr);
		if ( nSize > 0 ) {
			stat_free(nSize);
		}
	}
}

static void* wrapMemAlignHook(size_t nAlign, size_t nSize, const void* caller)
{
	void*	ptr;

	ptr = je_aligned_alloc(nAlign, nSize);
	if ( ptr != NULL )  {
		stat_alloc(je_sallocx(ptr, 0));
	}
	else {
		if ( nSize > 0 ) {
			stat_failed();
		}
	}

	return ptr;
}

static void initMemoryHook()
{
	__malloc_hook = wrapMallocHook;
	__realloc_hook = wrapReallocHook;
	__free_hook = wrapFreeHook;
	__memalign_hook = wrapMemAlignHook;
}

#ifndef __MALLOC_HOOK_VOLATILE
#define __MALLOC_HOOK_VOLATILE
#endif

void (*__MALLOC_HOOK_VOLATILE __malloc_initialize_hook) (void) = initMemoryHook;

#else /* USE_DEPRECATED_HOOK */

void* malloc(size_t nSize)
{
	void*	ptr;

	ptr = je_malloc(nSize);
	if ( ptr != NULL )  {
		stat_alloc(je_sallocx(ptr, 0));
	}
	else {
		if ( nSize > 0 ) {
			stat_failed();
		}
	}

	return ptr;
}

void* calloc (size_t nMemb, size_t nSize)
{
	void*	ptr;

	ptr = je_calloc(nMemb, nSize);
	if ( ptr != NULL )  {
		stat_alloc(je_sallocx(ptr, 0));
	}
	else {
		if ( nSize > 0 ) {
			stat_failed();
		}
	}

	return ptr;
}

void* realloc(void* ptr, size_t nNewSize)
{
	void*	ptrNew;

	if ( ptr != NULL )  {
		ssize_t		nOldSize, nNewReal;

		nOldSize = je_sallocx(ptr, 0);

		ptrNew = je_realloc(ptr, nNewSize);
		if ( nNewSize > 0 ) {
			if ( ptrNew != NULL ) {
				nNewReal = je_sallocx(ptrNew, 0);
				stat_realloc(nNewReal - nOldSize);
			}
			else {
				stat_failed();
			}
		}
		else {
			stat_free((size_t)nOldSize);
		}
	}
	else {
		ptrNew = malloc(nNewSize);
	}

	return ptrNew;
}

void free(void* ptr)
{
	if ( ptr != NULL )  {
		size_t	nSize;

		nSize = je_sallocx(ptr, 0);

		je_free(ptr);
		if ( nSize > 0 ) {
			stat_free(nSize);
		}
	}
}

void cfree(void* ptr)
{
	free(ptr);
}

void* memalign(size_t nAlign, size_t nSize)
{
	void*	ptr;

	ptr = je_aligned_alloc(nAlign, nSize);
	if ( ptr != NULL )  {
		stat_alloc(je_sallocx(ptr, 0));
	}
	else {
		if ( nSize > 0 ) {
			stat_failed();
		}
	}

	return ptr;
}

void* valloc(size_t nSize)
{
	void*	ptr;

	ptr = je_valloc(nSize);
	if ( ptr != NULL )  {
		stat_alloc(je_sallocx(ptr, 0));
	}
	else {
		if ( nSize > 0 ) {
			stat_failed();
		}
	}

	return ptr;
}

void* pvalloc(size_t nSize)
{
	void*	ptr;
	size_t	nRealSize = PAGE_ALIGN(nSize);

	ptr = valloc(nRealSize);
	return ptr;
}

#endif /* USE_DEPRECATED_HOOK */

#endif /* CARBON_JEMALLOC */
