/*
 *  Shell library
 *  Stdout appender for the logger
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.08.2015 16:27:30
 *      Initial revision.
 */

#include "shell/logger/appender_stdout.h"

/*
 * Initialise an appender
 *
 * Return: ESUCCESS
 */
result_t CAppenderStdout::init()
{
	flushAppender();
	return ESUCCESS;
}

/*
 * Free an appender resources
 */
void CAppenderStdout::terminate()
{
	flushAppender();
}

/*
 * Send a single string to the appender target
 *
 * 		pData		data to write
 * 		nLength		length of data, bytes
 *
 * Return: ESUCCESS, EIO
 */
result_t CAppenderStdout::append(const void* pData, size_t nLength)
{
	const char*	s = (const char*)pData;
	size_t		len, l;
	result_t	nresult = ESUCCESS;

	len = 0;

	while ( len < nLength )  {
		l = ::fwrite(&s[len], nLength-len, 1, m_outStream);
		if ( l == (nLength-len) )  {
			len += l;
		}
		else {
			/* Error write */
			nresult = EIO;
			clearerr(m_outStream);
			break;
		}
	}

	flushAppender();

	return nresult;
}
