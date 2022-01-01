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

#ifndef __SHELL_LOGGER_APPENDER_SYSLOG_H_INCLUDED__
#define __SHELL_LOGGER_APPENDER_SYSLOG_H_INCLUDED__

#include "shell/logger/appender.h"
#include "shell/logger/logger.h"

class CAppenderSyslog : public CAppender
{
	private:
		char			m_strIdent[64];

	public:
		CAppenderSyslog(const char* strIdent) : CAppender()
		{
			CLogger::copyString(m_strIdent, strIdent, sizeof(m_strIdent));
		}

		virtual ~CAppenderSyslog() noexcept = default;

	public:
		virtual result_t init();
		virtual void terminate();
		virtual result_t append(const void* pData, size_t nLength);
};

#endif /* __SHELL_LOGGER_APPENDER_SYSLOG_H_INCLUDED__ */
