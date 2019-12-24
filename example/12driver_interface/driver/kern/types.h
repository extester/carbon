/*
 *  Kernel library
 *  Library wide Types
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 10.03.2015 17:54:07
 *      Initial revision.
 */

#ifndef __KERN_TYPES_H_INCLUDED__
#define __KERN_TYPES_H_INCLUDED__

#include <linux/types.h>

#if defined(__LP64__) && __LP64__ == 1
typedef int64_t			natural_t;
typedef uint64_t		unatural_t;
typedef void*			ptr_t;
#else
typedef int32_t			natural_t;
typedef uint32_t		unatural_t;
typedef void*			ptr_t;
#endif

typedef int				boolean_t;

#ifndef TRUE
#define TRUE			(1==1)
#endif /* TRUE */
#ifndef FALSE
#define FALSE			(!TRUE)
#endif /* FALSE */

typedef int				result_t;
#define ESUCCESS		((result_t)0)

#ifndef NULL
#define NULL			((void*)0)
#endif /* NULL */

typedef uint32_t	unid_t;

#endif /* __KERN_TYPES_H_INCLUDED__ */
