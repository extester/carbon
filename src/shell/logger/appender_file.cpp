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
 *  Revision 1.0, 25.08.2015 17:16:55
 *      Initial revision.
 */

#include <unistd.h>
#include <sys/stat.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/logger/appender_file.h"


/*
 * Initialise an appender
 *
 * Return: ESUCCESS
 */
result_t CAppenderFile::init()
{
	struct stat statBuf;
	int         retVal;

	retVal = ::stat(m_strFilename, &statBuf);
	if ( retVal == 0 )  {
		m_nSize = (size_t)statBuf.st_size;
	}
	else  {
		if ( errno != ENOENT )  {
			logger_syslog_impl("FILE APPENDER INIT ERROR: failed to get log file '%s' size\n",
							   m_strFilename);
		}
		m_nSize = 0;
	}

	if ( m_nFileCountMax < 1 )  {
		m_nFileCountMax = 1;
	}

	return ESUCCESS;
}

/*
 * Free an appender resources
 */
void CAppenderFile::terminate()
{
}

/*
 * Make appender file string
 *
 * 		strFilename			appender full filename [out]
 * 		index				appender file index (0..m_nFileCountMax)
 *
 */
void CAppenderFile::makeFilename(char* strFilename, int index)
{
	if ( index > 0 )  {
		_tsnprintf(strFilename, APPENDER_FILENAME_MAX, "%s.%d", m_strFilename, index);
	}
	else {
		logger_copy_string_impl(strFilename, m_strFilename, APPENDER_FILENAME_MAX);
	}
}

/*
 * Send a single string to the appender
 *
 * 		pData		data to write
 * 		nLength		length of data, bytes
 *
 * Return: ESUCCESS, EIO, ...
 */
result_t CAppenderFile::append(const void* pData, size_t nLength)
{
	static boolean_t	wasError = FALSE;
	result_t			nresult;
	int					retVal;

	nresult = logger_append_file(m_strFilename, pData, nLength);
	if ( nresult != ESUCCESS ) {
		return nresult;
	}

	m_nSize += nLength;
	if ( m_nSize >= m_nFileSizeMax )  {
		/* Log file reaches the maximum size */
		char	strFilename1[APPENDER_FILENAME_MAX];
		char	strFilename2[APPENDER_FILENAME_MAX];

		makeFilename(strFilename1, (int)m_nFileCountMax-1);
		//printf("LOGGER: delete file: %s\n", strFilename1); fflush(stdout);
		::unlink(strFilename1);

		if ( m_nFileCountMax > 1 )  {
			size_t 	i;

			for(i=m_nFileCountMax-1; i>0; i--)  {
				makeFilename(strFilename1, (int)i-1);
				makeFilename(strFilename2, (int)i);
				//printf("LOGGER: rename file: %s => %s\n", strFilename1, strFilename2); fflush(stdout);
				retVal = ::rename(strFilename1, strFilename2);
				if ( retVal < 0 && wasError == FALSE )  {
					nresult = nresult == ESUCCESS ? errno : nresult;

					logger_syslog_impl("FILE APPENDER ERROR: can't rename '%s' => '%s', result=%d\n",
								  strFilename1, strFilename2, errno);
					wasError = TRUE;
				}
			}
		}

		m_nSize = 0;
	}

	return nresult;
}

/*
 * Add FILE appender to the existing logger
 *
 *		strFilename			full filename
 *		nFileCountMax		maximum files to store
 *		nFileSizMax			maximum a log file size, bytes
 *
 * Return: appender handle or LOGGER_APPENDER_NULL
 */
appender_handle_t logger_addFileAppender(const char* strFilename,
								size_t nFileCountMax, size_t nFileSizeMax)
{
	CAppenderFile*		pAppender;
	appender_handle_t	hAppender;

	pAppender = new CAppenderFile(strFilename, nFileCountMax, nFileSizeMax);
	hAppender = logger_insert_appender_impl(pAppender);
	if ( hAppender == LOGGER_APPENDER_NULL )  {
		/* Failed */
		delete pAppender;
	}

	return hAppender;
}
