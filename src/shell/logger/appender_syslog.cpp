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
#include "shell/logger/appender_syslog.h"

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
 * 		pData			data to write
 * 		nLength			length of data, bytes
 *
 * Return: ESUCCESS
 */
result_t CAppenderSyslog::append(const void* pData, size_t nLength)
{
	char*		strBuf;
	size_t		len;

	len = sh_min(LOGGER_BUFFER_MAX, nLength);
	strBuf = (char*)alloca(len);
	CLogger::copyString(strBuf, (const char*)pData, len);

	::syslog(SYSLOG_PRIORITY, "%s", strBuf);

	return ESUCCESS;
}
