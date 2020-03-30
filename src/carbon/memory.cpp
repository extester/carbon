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

#include "shell/defines.h"		/* For PAGE_SIZE */
#include "shell/assert.h"

#include "carbon/logger.h"
#include "carbon/memory.h"


static memory_stat_t	g_stat = {
	ZERO_ATOMIC, ZERO_ATOMIC, ZERO_ATOMIC, ZERO_ATOMIC,
	ZERO_ATOMIC, ZERO_ATOMIC, ZERO_ATOMIC
};

/*******************************************************************************
 * Memory Allocation Manager
 */

void CMemoryManager::getStat(void* pBuffer, size_t nSize) const
{
	size_t rsize = sh_min(nSize, sizeof(g_stat));

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

#if CARBON_MALLOC

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

#define EXT_SZ				(sizeof(extp_t))
#define TO_EXT(__ptr)		((extp_t*)(((char*)(__ptr))-EXT_SZ))

#define EXT_RED1			0x12345678
#define EXT_RED2			0x87654321

typedef struct {
#if CARBON_DEBUG_ASSERT
	natural_t 	red1;
#endif /* CARBON_DEBUG_ASSERT */
	size_t		size;
	void*		ptr;
#if CARBON_DEBUG_ASSERT
	natural_t 	red2;
#endif /* CARBON_DEBUG_ASSERT */
} __attribute__ ((packed)) extp_t;

/*
 * Function allocates size bytes and returns a pointer to the allocated memory.
 * The memory is not initialized. If size is 0, then function returns NULL
 *
 * Return: pointer to memory or NULL
 */
void* malloc(size_t nSize)
{
	void*		ptr = NULL;
	extp_t*		extp;

	if ( nSize > 0 )  {
		extp = (extp_t*)__libc_malloc(nSize+EXT_SZ);
		if ( extp )  {
			ptr = &extp[1];
			extp->size = nSize;
			extp->ptr = extp;
#if CARBON_DEBUG_ASSERT
			extp->red1 = EXT_RED1;
			extp->red2 = EXT_RED2;
#endif /* CARBON_DEBUG_ASSERT */
			stat_alloc(nSize);
		}
		else {
			stat_failed();
		}
	}

	return ptr;
}

/*
 * The function allocates memory for an array of nMemb elements of nSize bytes each
 * and returns a pointer to the allocated memory. The memory is set to zero.
 * If nMemb or size is 0, then function returns either NULL.
 *
 * Return: pointer to memory or NULL
 */
void* calloc(size_t nMemb, size_t nSize)
{
	void*		ptr = NULL;
	extp_t*		extp;
	size_t 		nBlockSize = nMemb*nSize;

	if ( nBlockSize > 0 ) {
		extp = (extp_t*)__libc_calloc(1, nBlockSize+EXT_SZ);
		if ( extp )  {
			ptr = &extp[1];
			extp->size = nBlockSize;
			extp->ptr = extp;
#if CARBON_DEBUG_ASSERT
			extp->red1 = EXT_RED1;
			extp->red2 = EXT_RED2;
#endif /* CARBON_DEBUG_ASSERT */
			stat_alloc(nBlockSize);
		}
		else {
			stat_failed();
		}
	}

	return ptr;
}

/*
 * The function changes the size of the memory block pointed to by ptr to nSize bytes.
 *
 * The contents will be unchanged in the range from the start of the region up to
 * the minimum of the old and new sizes.
 *
 * If the new size is larger than the old size, the added memory will not
 * be initialized.
 *
 * If ptr is NULL, then the call is equivalent to malloc(size),
 * for all values of size.
 *
 * If size is equal to zero, and ptr is not NULL, then the call is equivalent
 * to free(ptr).
 *
 * Unless ptr is NULL, it must have been returned by an earlier call to malloc(),
 * calloc() or realloc().
 *
 * Return: pointer to new memory or NULL
 */
void* realloc(void* ptr, size_t nSize) throw()
{
	void*		ptrNew = NULL;
	extp_t*		extp;
	size_t		nPrevSize;

	if ( ptr )  {
		if ( nSize > 0 )  {
			extp = TO_EXT(ptr);
			nPrevSize = extp->size;
			extp = (extp_t*)__libc_realloc(extp->ptr, nSize+EXT_SZ);
			if ( extp )  {
				ptrNew = &extp[1];
				extp->size = nSize;
				extp->ptr = extp;
#if CARBON_DEBUG_ASSERT
				extp->red1 = EXT_RED1;
				extp->red2 = EXT_RED2;
#endif /* CARBON_DEBUG_ASSERT */
				stat_realloc(nSize - nPrevSize);
			}
			else {
				stat_failed();
			}
		}
		else {
			free(ptr);
			/* Returning NULL */
		}
	}
	else {
		ptrNew = malloc(nSize);
	}

	return ptrNew;
}

/*
 * The function frees the memory space pointed to by ptr, which must have been
 * returned by a previous call to malloc(), calloc() or realloc().
 * Otherwise, or if free(ptr) has already been called before, undefined
 * behavior occurs. If ptr is NULL, no operation is performed.
 *
 * Return: none
 */
void free(void* ptr) throw()
{
	extp_t*		extp;
	size_t		nSize;

	if ( ptr )  {
		extp = TO_EXT(ptr);
#if CARBON_DEBUG_ASSERT
		shell_assert_ex(extp->red1 == EXT_RED1, "free(): red1 is corrupt");
		shell_assert_ex(extp->red2 == EXT_RED2, "free(): red2 is corrupt");
#endif /* CARBON_DEBUG_ASSERT */

		nSize = extp->size;
		__libc_free(extp->ptr);
		stat_free(nSize);
	}
}

static boolean_t __isPowerOfTwo(size_t n)
{
	return n != 0 && ((n&(n-1)) == 0);
}

static void* __aligned_alloc(size_t nAlign, size_t nSize)
{
	void		*ptr = NULL, *p;
	extp_t*		extp;
	size_t		ext_sz;

	ext_sz = ALIGN(EXT_SZ, nAlign);

	p = __libc_memalign(nAlign, nSize + ext_sz);
	if ( p )  {
		ptr = ((char*)p) + ext_sz;
		extp = TO_EXT(ptr);
		extp->ptr = p;
		extp->size = nSize;
#if CARBON_DEBUG_ASSERT
		extp->red1 = EXT_RED1;
		extp->red2 = EXT_RED2;
#endif /* CARBON_DEBUG_ASSERT */
		stat_alloc(nSize);
	}
	else {
		stat_failed();
	}

	return ptr;
}

/*
 * The obsolete function memalign() allocates nSize bytes and returns
 * a pointer to the allocated memory. The memory address will be
 * a multiple of alignment, which must be a power of two.
 *
 * Return: pointer to memory or NULL
 */
void* memalign(size_t nAlign, size_t nSize) throw()
{
	void*		ptr;

	if ( !__isPowerOfTwo(nAlign) || nSize == 0 )  {
		return NULL;
	}

	ptr = __aligned_alloc(nAlign, nSize);
	return ptr;
}

/*
 * The function allocates nAize bytes and places the address of the allocated
 * memory in *ptr.
 * The address of the allocated memory will be a multiple of alignment,
 * which must be a power of two and a multiple of sizeof(void *).
 * If size is 0, then function returns either NULL
 *
 * Return:
 * 		0 and memory address in ptr on success
 * 		EINVAL invalid parameters
 * 		ENOMEM memory shortage
 */
int posix_memalign(void **ptr, size_t nAlign, size_t nSize) throw()
{
	void*	ptr2;
	int		nres;

	if ( !__isPowerOfTwo(nAlign) || nSize == 0 )  {
		return EINVAL;
	}

	if ( (nSize&(sizeof(void*)-1)) != 0 )  {
		return EINVAL;
	}

	ptr2 = __aligned_alloc(nAlign, nSize);
	if ( ptr2 )  {
		*ptr = ptr2;
		nres = 0;
	}
	else {
		*ptr = NULL;
		nres = ENOMEM;
	}

	return nres;
}

/*
 * The function memalign() allocates nSize bytes and returns
 * a pointer to the allocated memory. The memory address will be
 * a multiple of alignment, which must be a power of two.
 *
 * The size should not be 0 and should be a multiple of nAlign
 *
 * Return: pointer to memory or NULL
 */
void* aligned_alloc(size_t nAlign, size_t nSize) throw()
{
	void*		ptr;

	if ( !__isPowerOfTwo(nAlign) || nSize == 0 )  {
		return NULL;
	}

	if ( (nSize&(nAlign-1)) != 0 )  {
		return NULL;
	}

	ptr = __aligned_alloc(nAlign, nSize);
	return ptr;

}

/*
 * The obsolete function valloc() allocates nSize bytes and returns a pointer
 * to the allocated memory. The memory address will be a multiple
 * of the page size. It is equivalent to memalign(sysconf(_SC_PAGESIZE),size).
 *
 * Return: pointer to memory or NULL
 */
void* valloc(size_t nSize) throw()
{
	void*	ptr;

	ptr = memalign(PAGE_SIZE, nSize);

	return ptr;
}

void* pvalloc(size_t nSize) throw()
{
	void*	ptr;
	size_t	nRealSize = PAGE_ALIGN(nSize);

	ptr = valloc(nRealSize);
	return ptr;
}


#endif /* CARBON_MALLOC */
