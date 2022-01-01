/*
 *  Shell library
 *  Global Data Types
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.03.2013 19:32:38
 *      Initial revision.
 */

#ifndef __SHELL_TYPES_H_INCLUDED__
#define __SHELL_TYPES_H_INCLUDED__

#include <stdint.h>
#include <stddef.h>
#if __unix__ && defined(__cplusplus)
#include <cinttypes>
#endif /* __unix__ && __cplusplus */

#if __WORDSIZE == 32
typedef int32_t			natural_t;
typedef uint32_t		unatural_t;
#elif __WORDSIZE == 64
typedef int64_t			natural_t;
typedef uint64_t		unatural_t;
#elif __embed__
typedef int32_t			natural_t;
typedef uint32_t		unatural_t;
#else
#error unsupported processor word size
#endif

typedef void*			ptr_t;
typedef natural_t		boolean_t;

#ifndef TRUE
#define TRUE			((boolean_t)(1==1))
#endif /* TRUE */
#ifndef FALSE
#define FALSE			((boolean_t)(!TRUE))
#endif /* FALSE */

typedef int32_t			result_t;

#ifndef NULL
#define NULL			((void*)0)
#endif /* NULL */

/*
 * Version format: major.minor.patch, date type is 32-bit unsigned:
 *
 * 	                 major    minor    patch
 * 		-------------------------------------
 * 		|00000000|xxxxxxxx|yyyyyyyy|zzzzzzzz|
 * 		-------------------------------------
 */
#define MAKE_VERSION(__major, __minor, __patch) 	\
	((version_t)( ((__patch)&0xff) | (((__minor)&0xff)<<8) | (((__major)&0xff)<<16) ))

#define VERSION_PATCH(__version)		((__version)&0xff)
#define VERSION_MINOR(__version)		(((__version)>>8)&0xff)
#define VERSION_MAJOR(__version)		(((__version)>>16)&0xff)

typedef uint32_t    version_t;

#endif /* __SHELL_TYPES_H_INCLUDED__ */
