/*
 *  Shell library
 *  Syslog appender for the logger
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.08.2015 16:53:42
 *      Initial revision.
 */

#include <syslog.h>
#include <alloca.h>

#include "shell/defines.h"
#include "shell/error.h"
#include "shell/logger.h"
#include "shell/logger/logger_base.h"


class CAppenderSyslog : public CAppender
{
	private:
		char			m_strIdent[64];

	public:
		CAppenderSyslog(const char* strIdent) : CAppender() {
			logger_copy_string_impl(m_strIdent, strIdent, sizeof(m_strIdent));
		}

		virtual ~CAppenderSyslog() {
		}

	public:
		virtual result_t init();
		virtual void terminate();
		virtual result_t append(const void* pData, size_t nLength);
};


/*
 * Initialise an appender
 *
 * Return: ESUCCESS
 */
result_t CAppenderSyslog::init()
{
	::openlog(m_strIdent, SYSLOG_OPTIONS, SYSLOG_FACILITY);
	return ESUCCESS;
}

/*
 * Free an appender resources
 */
void CAppenderSyslog::terminate()
{
	::closelog();
}

/*
 * Send a single string to the appender target
 *
 * 		pData		data to write
 * 		length		length of data, bytes
 *
 * Return: ESUCCESS
 */
result_t CAppenderSyslog::append(const void* pData, size_t nLength)
{
	char*		strBuf;
	size_t		len;

	len = sh_min(LOGGER_BUFFER_MAX, nLength);
	strBuf = (char*)alloca(len);
	logger_copy_string_impl(strBuf, (const char*)pData, len);

	::syslog(SYSLOG_PRIORITY, "%s", strBuf);

	return ESUCCESS;
}

/*
 * Add SYSLOG appender to the existing logger
 *
 *		strIdent			syslog ident string
 *
 * Return: appender handle or LOGGER_APPENDER_NULL
 */
appender_handle_t logger_addSyslogAppender(const char* strIdent)
{
	CAppenderSyslog*	pAppender;
	appender_handle_t	hAppender;

	pAppender = new CAppenderSyslog(strIdent);
	hAppender = logger_insert_appender_impl(pAppender);
	if ( hAppender == LOGGER_APPENDER_NULL )  {
		/* Failed */
		delete pAppender;
	}

	return hAppender;
}
