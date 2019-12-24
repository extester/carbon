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

#include <stdio.h>

#include "shell/error.h"
#include "shell/logger.h"
#include "shell/logger/logger_base.h"


class CAppenderStdout : public CAppender
{
	private:
		FILE*			m_outStream;		/* Output stream */

	public:
		CAppenderStdout() : CAppender() {
			m_outStream = stdout;
		}

		virtual ~CAppenderStdout() {
			flushAppender();
		}

	public:
		virtual result_t init();
		virtual void terminate();
		virtual result_t append(const void* pData, size_t nLength);

	private:
		void flushAppender() {
			::fflush(m_outStream);
		}
};

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

/*
 * Add STDOUT appender to the existing logger
 *
 * Return: appender handle or LOGGER_APPENDER_NULL
 */
appender_handle_t logger_addStdoutAppender()
{
	CAppenderStdout*	pAppender;
	appender_handle_t	hAppender;

	pAppender = new CAppenderStdout();
	hAppender = logger_insert_appender_impl(pAppender);
	if ( hAppender == LOGGER_APPENDER_NULL )  {
		/* Failed */
		delete pAppender;
	}

	return hAppender;
}
