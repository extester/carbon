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

#include "shell/tstdio.h"
#include "shell/logger/appender_file.h"

/*
 * Initialise an appender
 *
 * Return: ESUCCESS
 */
result_t CAppenderFile::init()
{
	struct stat 	statBuf;
	int         	retVal;

	retVal = ::stat(m_strFilename, &statBuf);
	if ( retVal == 0 )  {
		m_nSize = (size_t)statBuf.st_size;
	}
	else  {
		if ( errno != ENOENT )  {
			CLogger::syslog("FILE APPENDER INIT ERROR: failed to get log file '%s' size\n",
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
void CAppenderFile::makeFilename(char* strFilename, int index) const
{
	if ( index > 0 )  {
		_tsnprintf(strFilename, APPENDER_FILENAME_MAX, "%s.%d", m_strFilename, index);
	}
	else {
		CLogger::copyString(strFilename, m_strFilename, APPENDER_FILENAME_MAX);
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

	nresult = CLogger::appendFile(m_strFilename, pData, nLength);
	if ( nresult != ESUCCESS ) {
		return nresult;
	}

	m_nSize += nLength;
	if ( m_nSize >= m_nFileSizeMax )  {
		/* Log file reaches the maximum size */
		char	strFilename1[APPENDER_FILENAME_MAX];
		char	strFilename2[APPENDER_FILENAME_MAX];

		makeFilename(strFilename1, (int)m_nFileCountMax-1);
		::unlink(strFilename1);

		if ( m_nFileCountMax > 1 )  {
			size_t 	i;

			for(i=m_nFileCountMax-1; i>0; i--)  {
				makeFilename(strFilename1, (int)i-1);
				makeFilename(strFilename2, (int)i);

				retVal = ::rename(strFilename1, strFilename2);
				if ( retVal < 0 && wasError == FALSE )  {
					nresult = nresult == ESUCCESS ? errno : nresult;

					CLogger::syslog("FILE APPENDER ERROR: can't rename '%s' => '%s', result: %d\n",
								  		strFilename1, strFilename2, errno);
					wasError = TRUE;
				}
			}
		}

		m_nSize = 0;
	}

	return nresult;
}
