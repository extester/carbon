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
 *  Revision 1.0, 03.04.2017 16:37:39
 *      Initial revision.
 */

#if 0
template <class T>
class CFunctorAllocatorTempl
{
	public:
		typedef size_t    size_type;
		typedef ptrdiff_t difference_type;
		typedef T*        pointer;
		typedef const T*  const_pointer;
		typedef T&        reference;
		typedef const T&  const_reference;
		typedef T         value_type;

	public:
		CFunctorAllocatorTempl() {}
		CFunctorAllocatorTempl(const CFunctorAllocatorTempl&) {}

		pointer allocate(size_type n, const void * = 0)
		{
		}

		void  deallocate(void* p, size_type)
		{
		}

		pointer address(reference x) const { return &x; }
		const_pointer address(const_reference x) const { return &x; }
		CFunctorAllocatorTempl<T>& operator=(const CFunctorAllocatorTempl&) { return *this; }

		void construct(pointer p, const T& val) {
			new ((T*) p) T(val);
		}

		void destroy(pointer p) { p->~T(); }
		size_type max_size() const { return size_t(-1); }

		template <class U>
		struct rebind { typedef CFunctorAllocatorTempl<U> other; };

		template <class U>
		CFunctorAllocatorTempl(const CFunctorAllocatorTempl<U>&) {}

		template <class U>
		CFunctorAllocatorTempl& operator=(const CFunctorAllocatorTempl<U>&) { return *this; }
};

#define FAKE_DATA_SIZE			12		/* bytes */

typedef struct
{
	uint8_t	fake[FAKE_DATA_SIZE];
} fake_data_t;

extern CFunctorAllocatorTempl<fake_data_t>	g_functorAllocator;

#endif