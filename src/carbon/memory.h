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
 *  Revision 1.0, 05.04.2015 22:10:01
 *      Initial revision.
 *
 *  Revision 2.0, 13.07.2015 14:47:37
 *  	Completle rewrote to use jemalloc library.
 *
 *  Revision 3.0, 19.10.2018 17:36:29
 *  	Removed deprecated glibc memory hooks and
 *  	redefine weak malloc/free functions.
 */

#ifndef __CARBON_MEMORY_H_INCLUDED__
#define __CARBON_MEMORY_H_INCLUDED__

#include "shell/config.h"
#include "shell/shell.h"
#include "shell/atomic.h"
#include "shell/memory.h"

#include "carbon/module.h"

/*
 * Statistic data for the Memory Manager
 */
typedef struct  {
	atomic_t	full_alloc_count;			/* Allocated blocks */
	atomic_t	free_count;					/* Freed blocks */
	atomic_t	alloc_count;				/* Currently allocated blocks */
	atomic_t	alloc_size;					/* Currently allocated size */
	atomic_t	alloc_size_max;				/* Max. simultaneously allocated size */
	atomic_t	realloc_count;				/* Reallocated blocks */
	atomic_t	fail_count;					/* Fail count */
} __attribute__ ((packed)) memory_stat_t;

/*
 * Memory manager
 */
class CMemoryManager : public CModule
{
	public:
		CMemoryManager() : CModule("MemoryManager") {}
		virtual ~CMemoryManager() {}

	public:
		/* Statistic functions */
		virtual size_t getStatSize() const { return sizeof(memory_stat_t); }
		virtual void getStat(void* pBuffer, size_t nSize) const;
		virtual void resetStat();

	public:
		virtual void dump(const char* strPref = "") const;
};

#if CARBON_MALLOC

extern "C" {

void* __libc_malloc(size_t nSize);
void* __libc_calloc(size_t nNumb, size_t nSize);
void* __libc_realloc(void* ptr, size_t nSize);
void __libc_free(void* ptr);
void* __libc_memalign(size_t nAlign, size_t nSize);

void* malloc(size_t nSize);
void* calloc(size_t nMemb, size_t nSize);
void* realloc(void* ptr, size_t nSize) throw();
void free(void* ptr) throw();

void* memalign(size_t nAlign, size_t nSize) throw();
int posix_memalign(void **ptr, size_t nAlign, size_t nSize) throw();
void* aligned_alloc(size_t nAlign, size_t size) throw();

void* valloc(size_t nSize) throw();
void* pvalloc(size_t nSize) throw();

};

#endif /* CARBON_MALLOC */

#endif /* __CARBON_MEMORY_H_INCLUDED__ */
