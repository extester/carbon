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

#ifndef __SHELL_LOGGER_APPENDER_STDOUT_H_INCLUDED__
#define __SHELL_LOGGER_APPENDER_STDOUT_H_INCLUDED__

#include <stdio.h>

#include "shell/logger/appender.h"
#include "shell/logger/logger.h"

class CAppenderStdout : public CAppender
{
	private:
		FILE*			m_outStream;		/* Output stream */

	public:
		CAppenderStdout() : CAppender() {
			m_outStream = stdout;
		}

		virtual ~CAppenderStdout() noexcept {
			flushAppender();
		}

	private:
		void flushAppender() {
			::fflush(m_outStream);
		}

	public:
		virtual result_t init();
		virtual void terminate();
		virtual result_t append(const void* pData, size_t nLength);
};

#endif /* __SHELL_LOGGER_APPENDER_STDOUT_H_INCLUDED__ */
