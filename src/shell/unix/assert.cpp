/*
 *  Shell library
 *	Library assertion (UNIX)
 *
 *  Copyright (c) 2013-2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.03.2013 15:29:17
 *      Initial revision.
 *
 *  Revision 1.1, 03.02.2015 13:17:26
 *  	Added shell_assert_ex()
 *
 *  Revision 1.2, 23.03.2017 15:53:20
 *  	Changed exit() code: 1 => 0xff
 */

#include "shell/assert.h"

#if CARBON_DEBUG_ASSERT

#include <stdlib.h>		/* for exit() */

#include "shell/logger.h"
#include "shell/tstring.h"
#include "shell/tstdio.h"
#include "shell/debug.h"


/*
 * Failed assertion function. Assertion message is written to STDERR
 *
 *		strExpr			asserted expression
 *		strFile			file name
 *		nLine			source file line
 */
void __shell_assert(const char* strExpr, const char* strFile, int nLine)
{
    char    buf[256];

    _tsnprintf(buf, sizeof(buf), "Assertion failed %s:%d: %s\n",
			   strFile, nLine, strExpr);

    log_dump(buf);
    log_dump("Backtrace:\n");
    printBacktrace();

    exit(0xff);
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
	char	buf[512];
	size_t	len;

	len = (size_t)_tsnprintf(buf, sizeof(buf), "Assertion failed %s:%d: %s",
							 strFile, nLine, strExpr);
	if ( len < sizeof(buf) )  {
		_tstrcat(buf, " "); len++;
		if ( len < sizeof(buf) )  {
	    	va_list	args;

	    	va_start(args, strMessage);
			_tvsnprintf(&buf[len], sizeof(buf)-len, strMessage, args);
	    	va_end(args);
		}
	}

    log_dump(buf);
    log_dump("Backtrace:\n");
    printBacktrace();

    exit(0xff);
}

#endif /* CARBON_DEBUG_ASSERT */
