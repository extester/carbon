/*
 *  Shell library
 *  Static buffers allocator
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

#include "shell/shell.h"
#include "shell/logger.h"

#ifndef __SHELL_STATIC_ALLOCATOR_H_INCLUDED__
#define __SHELL_STATIC_ALLOCATOR_H_INCLUDED__

extern void no_static_memory() __attribute__ ((noreturn));

template<size_t nElemSize, size_t nAlignSize>
class CStaticAllocator
{
	private:
		union Node {
			unsigned char data[ALIGN(nElemSize, nAlignSize)];
			Node* next;
		};

		Node* pFreeList;

	public:
		CStaticAllocator(void* pBuffer, size_t size);
		~CStaticAllocator();

	public:
		void* alloc();
		void dealloc(void* p);
};

template<size_t nElemSize, size_t nAlignSize>
CStaticAllocator<nElemSize, nAlignSize>::CStaticAllocator(void* pBuffer, size_t size) :
	pFreeList((Node*)pBuffer)
{
	size_t	i, count = size/nElemSize;

	shell_assert(count > 0);
	shell_assert(ALIGN(nElemSize, nAlignSize) == nElemSize);

	for(i=0; i<count-1; ++i) {
		pFreeList[i].next = &pFreeList[i+1];
	}

	pFreeList[count-1].next = 0;
};

template<size_t nElemSize, size_t nAlignSize>
CStaticAllocator<nElemSize, nAlignSize>::~CStaticAllocator()
{
}

template<size_t nElemSize, size_t nAlignSize>
void* CStaticAllocator<nElemSize, nAlignSize>::alloc()
{
	if ( pFreeList != 0 )  {
		void* pMem = pFreeList;
		pFreeList = pFreeList->next;
		return pMem;
	}

	return NULL;
}

template<size_t nElemSize, size_t nAlignSize>
void CStaticAllocator<nElemSize, nAlignSize>::dealloc(void* ptr)
{
	if ( ptr != NULL )  {
		Node* p = (Node*)ptr;
		p->next = pFreeList;
		pFreeList = p;
	}
}

#endif /* __SHELL_STATIC_ALLOCATOR_H_INCLUDED__ */
