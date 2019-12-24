/*
 *  Shell library
 *  File appender for the logger
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 26.08.2015 11:29:37
 *      Initial revision.
 */

#ifndef __SHELL_LOGGER_APPENDER_FILE_H_INCLUDED__
#define __SHELL_LOGGER_APPENDER_FILE_H_INCLUDED__

#include "shell/logger.h"
#include "shell/logger/logger_base.h"

#define APPENDER_FILENAME_MAX		256

class CAppenderFile : public CAppender
{
	protected:
		char			m_strFilename[APPENDER_FILENAME_MAX];	/* Full filename */
		size_t			m_nFileCountMax;		/* Maximum log files to maintain */
		size_t			m_nFileSizeMax;			/* Maximum log file size, bytes */
		size_t			m_nSize;				/* Current file size, bytes */

	public:
		CAppenderFile(const char* strFilename, size_t nFileCountMax, size_t nFileSizeMax) :
			CAppender(),
			m_nFileCountMax(nFileCountMax),
			m_nFileSizeMax(nFileSizeMax),
			m_nSize(0)
		{
			logger_copy_string_impl(m_strFilename, strFilename, sizeof(m_strFilename));
		}

		virtual ~CAppenderFile() {
		}

	public:
		virtual result_t init();
		virtual void terminate();
		virtual result_t append(const void* pData, size_t nLength);

	protected:
		void makeFilename(char* strFilename, int index);
};

#endif /* __SHELL_LOGGER_APPENDER_FILE_H_INCLUDED__ */
