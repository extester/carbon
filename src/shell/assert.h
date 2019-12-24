/*
 *  Shell library
 *	Library assertion
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *	Revision 1.0, 05.03.2013 18:29:58
 *      Initial revision.
 *
 *  Revision 1.1, 03.02.2015 17:29:04
 *  	Added shell_assert_ex()
 */

#ifndef __SHELL_ASSERT_H_INCLUDED__
#define __SHELL_ASSERT_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdarg.h>

#include "shell/config.h"

#if CARBON_DEBUG_ASSERT

/*
 * Assertion enabled
 */

#define shell_assert(__b)            \
    do {  \
        if ( !(__b) )  {  \
            __shell_assert(#__b, __FILE__, __LINE__);  \
        }  \
    } while(0)

#define shell_assert_ex(__b, __message, ...)    \
    do {  \
        if ( !(__b) )  { \
            __shell_assert_ex(#__b, __FILE__, __LINE__, __message, ##__VA_ARGS__); \
        }  \
    } while(0)

extern void __shell_assert(const char* strExpr, const char* strFile, int nLine);
extern void __shell_assert_ex(const char* strExpr, const char* strFile, int nLine,
				  const char* strMessage, ...);

#define shell_verify(__b)            shell_assert(__b)

#else /* CARBON_DEBUG_ASSERT */

/*
 * Assertion disabled
 */

#define shell_assert(__b)    					do {} while(0)
#define shell_assert_ex(__b, __message, ...)	do {} while(0)

#define shell_verify(__b)						((void)(__b))

#endif /* CARBON_DEBUG_ASSERT */

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __SHELL_ASSERT_H_INCLUDED__ */
