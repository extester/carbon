/*
 *  Shell library
 *	Library assertion (embedded version)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.03.2017 10:50:40
 *      Initial revision.
 */

#include "shell/defines.h"
#include "shell/assert.h"

#if CARBON_DEBUG_ASSERT

#include "shell/embed/console_output.h"

static void postAssertion() __attribute__ ((noreturn));
static void postAssertion()
{
	while(1) {}
}

/*
 * Failed assertion function
 *
 *		strExpr			asserted expression
 *		strFile			file name
 *		nLine			source file line
 */
void __shell_assert(const char* strExpr, const char* strFile, int nLine)
{
	console_printf("Assert %s:%d: %s\n", strFile, nLine, strExpr);
	postAssertion();
}

/*
 * Failed assetion function with extended formatted string
 *
 *		strExpr			asserted expression
 *		strFile			file name
 *		nLine			source file line
 *		strMessage		extra message
 */
void __shell_assert_ex(const char* strExpr, const char* strFile, int nLine,
					   const char* strMessage, ...)
{
	UNUSED(strMessage);
	__shell_assert(strExpr, strFile, nLine);
}

#endif /* CARBON_DEBUG_ASSERT */
