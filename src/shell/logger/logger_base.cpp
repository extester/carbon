/*
 *  Shell library
 *  Logger
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 26.01.2015 16:27:29
 *      Initial revision.
 *
 *  Revision 1.1, 25.02.2015
 *  	Added CAppenderTcpServer appender.
 *
 *	Revision 1.2, 03.04.2015 23:48:31
 *		Optimised logger_is_enabled() function.
 *
 *  Revision 1.3, 04.04.2015 21:47:26
 *  	Added L_ALWAYS_DISABLED log channel.
 *
 *  Revision 2.0, 25.08.2015 15:17:40
 *  	Separate appenders to the modules.
 *
 */
/*
 * 	Extra definitions:
 *
 * 		LOGGER_SINGLE_THREAD
 * 			Define to compile out multi-threading access.
 */
/*
 * 		CLogger
 * 		   |
 * 		   + -------- CAppenderStdout		(Log to stdout stream)
 * 		   |
 * 		   + -------- CAppenderFile			(Log to file)
 * 		   |
 * 		   + -------- CAppenderSyslog		(Log to syslog)
 * 		   |
 * 		   + -------- CAppenderTcpServer	(Log to remove client)
 * 		   |
 * 		   + -------- CAppenderPickup		(Log to file and cutting)
 * 		   |
 * 		   + ...
 */
/*
 * openlog(), syslog(), closelog(), setlogmask() notes:
 *
 * 	1) 	According http://www.gnu.org/software/libc/manual/html_node/openlog.html#openlog
 * 		syslog() is thread-safe;
 *
 * 	2)  closelog() is automatically called on exit() or exec();
 *
 * 	3)  Reopening is like opening except that if you specify zero for the default
 * 		facility code, the default facility code simply remains unchanged;
 *
 * 	4)  setlogmask() doesn't hide any messages by default.
 *
 */

#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <syslog.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/logger/logger_base.h"


#define GET_REAL_TYPES(__types)	\
	( (__types)==LT_ALL ? (LT_DEBUG|LT_ERROR|LT_WARNING|LT_INFO) : (__types) )


/*******************************************************************************
 * Utility functions
 */

/*
 * Rescue logger
 *
 * 	message		message to send to syslog
 */
void logger_syslog_impl(const char* strMessage, ...)
{
	va_list     		args;
	static boolean_t	initialised = FALSE;

	if ( !initialised )  {
		openlog(NULL, SYSLOG_OPTIONS, SYSLOG_FACILITY);
		initialised = TRUE;
	}

	va_start(args, strMessage);
	vsyslog(LOG_ERR, strMessage, args);
	va_end(args);
}

/*
 * Copy string
 *
 * 		dst		destination address
 * 		src		source string
 * 		n		destination buffer size
 */
void logger_copy_string_impl(char* dst, const char* src, size_t n)
{
    size_t l = _tstrlen(src);

    l = MIN(l, (n-1));
    UNALIGNED_MEMCPY(dst, src, l);
    dst[l] = '\0';
}

/*
 * Append buffer to a file
 *
 *      filename        full filename
 *      buf             buffer to write
 *      length          buffer length, bytes
 *
 * Return: ESUCCESS, ...
 *
 * Note: The logging function couldn't be used in the function
 * Note: use O_APPEND for atomically write
 */
result_t logger_append_file(const char* filename, const void* buf, size_t length)
{
    int         handle;
    result_t	nresult = ESUCCESS;
    ssize_t     count;

    handle = open(filename, O_CREAT|O_WRONLY|O_APPEND, (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
    if ( handle >= 0 )  {
        count = write(handle, buf, length);
        if ( count < (ssize_t)length )  {
            nresult = count < 0 ? errno : EIO;
        }

        close(handle);
    }
    else  {
        nresult = errno;
    }

    return nresult;
}

/*******************************************************************************
 * Logger class
 */

CLogger::CLogger()
{
	size_t	i;

#if !LOGGER_SINGLE_THREAD
	pthread_mutex_init(&m_lock, NULL);
#endif /* !LOGGER_SINGLE_THREAD */

	m_arAppender.reserve(16);

	for(i=0; i<ARRAY_SIZE(m_arLogger); i++) {
		_tmemset(&m_arLogger[i].arChannel, 0xff, sizeof(m_arLogger[i].arChannel));
		setFormat(LT_ALL, LOGGER_FORMAT_DEFAULT);
		setFormatTime(LT_ALL, LOGGER_FORMAT_TIME_DEFAULT);
	}

	logger_copy_string_impl(m_arLogger[I_DEBUG].strType, "DBG", sizeof(m_arLogger[I_DEBUG].strType));
	logger_copy_string_impl(m_arLogger[I_ERROR].strType, "ERR", sizeof(m_arLogger[I_ERROR].strType));
	logger_copy_string_impl(m_arLogger[I_WARNING].strType, "WRN", sizeof(m_arLogger[I_WARNING].strType));
	logger_copy_string_impl(m_arLogger[I_INFO].strType, "INF", sizeof(m_arLogger[I_INFO].strType));
}

CLogger::~CLogger()
{
	size_t	i, count = m_arAppender.size();

	lock();
	for (i=0; i<count; i++) {
		m_arAppender[i]->terminate();
		delete m_arAppender[i];
	}
	m_arAppender.clear();
	unlock();

#if !LOGGER_SINGLE_THREAD
	pthread_mutex_destroy(&m_lock);
#endif /* !LOGGER_SINGLE_THREAD */
}


/*
 * Insert new appender to the available appenders array
 *
 * 		pAppender			uninitialised appender object
 *
 * Return: handle or LOGGER_APPENDER_NULL
 */
appender_handle_t CLogger::insertAppender(CAppender* pAppender)
{
	appender_handle_t	hAppender = LOGGER_APPENDER_NULL;
	result_t			nresult;

	nresult = pAppender->init();
	if ( nresult == ESUCCESS )  {
		lock();
		m_arAppender.push_back(pAppender);
		unlock();
		hAppender = pAppender->getHandle();
	}

	return hAppender;
}

/*
 * Remove an appender
 *
 * 		hAppender		appender handle to remove
 *
 * Return: ESUCCESS, ENOENT
 */
result_t CLogger::removeAppender(appender_handle_t hAppender)
{
	CAppender*	pAppender = 0;
	size_t		count, i;

	lock();

	count = m_arAppender.size();
	for(i=0; i<count; i++)  {
		if ( m_arAppender[i]->getHandle() == hAppender )  {
			pAppender = m_arAppender[i];
			m_arAppender.erase(m_arAppender.begin()+i);
			break;
		}
	}

	unlock();

	if ( pAppender )  {
		pAppender->terminate();
		delete pAppender;
	}

	return pAppender ? ESUCCESS : ENOENT;
}


/*
 * Convert logger type index to logger type
 *
 * 		index		logger type index, 0..LOGGER_TYPES-1
 *
 * Return: logger type LT_xxx or 0 if none found
 */
unsigned int CLogger::i2type(unsigned int index) const
{
	static unsigned int	ar[] = { LT_DEBUG, LT_ERROR, LT_WARNING, LT_INFO };

	return index < ARRAY_SIZE(ar) ? ar[index] : 0;
}


/*
 * Convert logger type to logger type index
 *
 * 		type		logger type, LT_xxx
 *
 * Return: logger index 0..LOGGER_TYPES-1 or -1 if none found
 */
int CLogger::type2i(unsigned int type) const
{
	static unsigned int	ar[] = { LT_DEBUG, LT_ERROR, LT_WARNING, LT_INFO };
	size_t				i;
	int					index = -1;

	for(i=0; i<ARRAY_SIZE(ar); i++)  {
		if ( ar[i] == type )  {
			index = (int)i;
			break;
		}
	}

	return index;
}

/*
 * Set output format for the specified log type messages
 *
 * 		type			log types LT_xxx (may be LT_ALL)
 * 		strFormat		format string (formats see above)
 */
void CLogger::setFormat(unsigned int type, const char* strFormat)
{
	unsigned int	i, lt;

	lt = GET_REAL_TYPES(type);

	lock();
	for(i=0; i<LOGGER_TYPES; i++)  {
		if ( lt & i2type(i) )  {
			logger_copy_string_impl(m_arLogger[i].strFormat, strFormat,
									sizeof(m_arLogger[i].strFormat));
		}
	}
	unlock();
}

/*
 * Set output format for the timestamp
 *
 * 		type			log types LT_xxx (may be LT_ALL)
 * 		strFormat		format string
 */
void CLogger::setFormatTime(unsigned int type, const char* strFormat)
{
	unsigned int	i, lt;

	lt = GET_REAL_TYPES(type);

	lock();
	for(i=0; i<LOGGER_TYPES; i++)  {
		if ( lt & i2type(i) )  {
			logger_copy_string_impl(m_arLogger[i].strFormatTime, strFormat,
									sizeof(m_arLogger[i].strFormatTime));
		}
	}
	unlock();
}

/*
 * Enable logger types/channel
 *
 * 		type_channel		enabling log types/channels
 * 								type:		enable log types LT_xxx (may be LT_ALL)
 * 								channel: 	log channels to enable L_xxx (may be L_ALL)
 */
void CLogger::enable(unsigned int type_channel)
{
	unsigned int	i, ch = GET_CHANNEL0(type_channel);
	int 			lt = GET_REAL_TYPES(GET_TYPES(type_channel));
	int				index, bit, exc_index, exc_bit;

	exc_index = L_ALWAYS_DISABLED/8;
	exc_bit = 1 << (L_ALWAYS_DISABLED&7);

	index = ch/8;
	bit = 1 << (ch&7);

	lock();
	for(i=0; i<LOGGER_TYPES; i++)  {
		if ( lt & i2type(i) )  {
			if ( ch != 0 )  {
				m_arLogger[i].arChannel[index] |= bit;
			}
			else {
				memset(&m_arLogger[i].arChannel, 0xff, sizeof(m_arLogger[i].arChannel));
			}

			/* L_ALWAYS_DISABLED is always disabled */
			m_arLogger[i].arChannel[exc_index] &= ~exc_bit;
		}
	}
	unlock();
}

/*
 * Disable logger types/channel
 *
 * 		type_channel		disabling log types/channels
 * 								type:		enable log types LT_xxx (may be LT_ALL)
 * 								channel: 	log channels to disable L_xxx (may be L_ALL)
 */
void CLogger::disable(unsigned int type_channel)
{
	unsigned int	i, ch = GET_CHANNEL0(type_channel);
	int 			lt = GET_REAL_TYPES(GET_TYPES(type_channel));
	int				index, bit, exc_index, exc_bit;

	exc_index = L_ALWAYS_ENABLED/8;
	exc_bit = 1 << (L_ALWAYS_ENABLED&7);

	index = ch/8;
	bit = 1 << (ch&7);

	lock();
	for(i=0; i<LOGGER_TYPES; i++)  {
		if ( lt & i2type(i) )  {
			if ( ch != 0 )  {
				m_arLogger[i].arChannel[index] &= ~bit;
			}
			else {
				_tmemset(&m_arLogger[i].arChannel, 0x00, sizeof(m_arLogger[i].arChannel));
			}

			/* L_ALWAYS_ENABLED is always enabled */
			m_arLogger[i].arChannel[exc_index] |= exc_bit;
		}
	}
	unlock();
}

/*
 * Check if a specified log type/channel is enabled
 *
 * 		type_channel		log type/channel to check (LT_xxx | L_xxx)
 *
 * Return:
 * 		TRUE: 	log is enabled
 * 		FALSE: 	log is disabled
 */
boolean_t CLogger::isEnabled(unsigned int type_channel)
{
	unsigned int	ch = GET_CHANNEL0(type_channel);
	unsigned int	lt = GET_TYPES(type_channel);
	boolean_t		bEnabled = FALSE;

	if ( ch != 0 && lt != 0 )  {
		unsigned int 	i, index, bit;

		index = ch/8;
		bit = (unsigned int)1 << (ch&7);

		lock();
		for(i=0; i<LOGGER_TYPES; i++)  {
			if ( lt & i2type(i) )  {
				bEnabled = (m_arLogger[i].arChannel[index]&bit) != 0;
				break;
			}
		}
		unlock();
	}

	return bEnabled;
}

/*
 * Check if a specified channel is enabled
 *
 * 		pLogger		logger channel pointer
 * 		ch			channel bitmap to check
 *
 * Return:
 * 		TRUE: 	log is enabled
 * 		FALSE: 	log is disabled
 */
boolean_t CLogger::isEnabledType(const logger_type_t* pLogger, unsigned int ch) const
{
	int 		index, bit;
	boolean_t	bEnabled = FALSE;

	if ( ch != 0 )  {
		index = ch/8;
		bit = 1 << (ch&7);

		bEnabled = (pLogger->arChannel[index]&bit) != 0;
	}

	return bEnabled;
}

/*
 * Format logger string
 *
 * 		pLogger				current logger
 * 		strLoggerFormat		string format
 * 		strMessage			user message
 * 		strFilename			source filename (may be NULL)
 * 		nLine				source line (1..n)
 * 		strFunction			source function (may be NULL)
 * 		strBuffer			output  buffer
 * 		nBufferLength		output buffer size
 * 		args				VA arguments
 *
 * Return: output string length (excluding trailing '\0')
 */
size_t CLogger::doFormat(const logger_type_t* pLogger, const char* strLoggerFormat,
			const char* strMessage, const char* strFilename, int nLine,
			const char* strFunction, char* strBuffer, size_t nBufferLength,
			va_list args) const
{
	const char*	f = strLoggerFormat;
	char*		s = strBuffer;
	size_t		len = nBufferLength-1, l;
	char		c;
    time_t      time_now;
    struct tm   tm;

	while ( (c=*f++) != '\0' && len > 0 )  {
		/*
		 * Is not control sequence
		 */
		if ( c != '%' )  {
			*s = c;
			s++; len--;
			continue;
		}

		switch ( (c=*f++) )  {

			case 'n':				/* Carriage return */
				*s = '\n';
				s++; len--;
				break;

			case 'F':				/* Source filename */
				_tsnprintf(s, len, "%s", strFilename ? strFilename : "<null>");
				l = _tstrlen(s);
				s += l; len -= l;
				break;

			case 'N':				/* Source filename line number */
				_tsnprintf(s, len, "%d", nLine);
				l = _tstrlen(s);
				s += l; len -= l;
				break;

			case 'P':				/* Source function name */
				_tsnprintf(s, len, "%s", strFunction ? strFunction : "<null>");
				l = _tstrlen(s);
				s += l; len -= l;
				break;

			case 'T':				/* Timestamp */
		        time_now = time(NULL);
		        localtime_r(&time_now, &tm);
		        l = strftime(s, len, pLogger->strFormatTime, &tm);
				s += l; len -= l;
				break;

			case 's':				/* User message */
				_tvsnprintf(s, len, strMessage, args);
				l = _tstrlen(s);
				s += l; len -= l;
				break;

			case 'l':				/* Log type: 'INF', 'WRN', 'ERR', 'DBG' */
				_tsnprintf(s, len, "%s", pLogger->strType);
				l = _tstrlen(s);
				s += l; len -= l;
				break;

			case 'p':				/* Process ID */
				_tsnprintf(s, len, "%d", getpid());
				l = _tstrlen(s);
				s += l; len -= l;
				break;

#if !LOGGER_SINGLE_THREAD
			case 't':				/* Pthread ID */
				_tsnprintf(s, len, "%X", (unsigned int)pthread_self());
				l = _tstrlen(s);
				s += l; len -= l;
				break;
#endif /* !LOGGER_SINGLE_THREAD */

			case '%':
				*s = '%';
				s++; len--;
				break;

			default:
				*s = '%';
				s++; len--;
				if ( len > 0 )  {
					*s = c;
					s++; len--;
				}
				break;
		}
	}

	*s = '\0';
	return A(s) - A(strBuffer);
}

/*
 * Format logger HEX dump
 *
 * 		pLogger				current logger
 * 		pData				data to dump
 * 		nLength				data length, bytes
 * 		strBuffer			output buffer
 * 		nBufferLength		output buffer size
 *
 * Return: output buffer length (excluding trailing '\0')
 */
size_t CLogger::doFormatHex(const logger_type_t* pLogger, const void* pData, int nLength,
						char* strBuffer, size_t nBufferLength) const
{
	const char*		strPref;
	const char*		p = (const char*)pData;
	char*			s = strBuffer;
	int				i, l;
	int				len = (int)nBufferLength-1;

	if ( pData != 0 && nLength > 0 )  {
		for(i=0; i<nLength; i++)  {
			if ( len <= 0 ) {
				break;
			}

			if ( i == 0 ) 		strPref = " "; else
			if ( (i&15) == 0 ) 	strPref = "\n"; else
			if ( (i&3) == 0 ) 	strPref = "  "; else strPref = " ";

			_tsnprintf(s, (size_t)len, "%s%02X", strPref, (*p)&0xff);
			l = (int)_tstrlen(s);
			s += l; len -= l; p++;
		}
	}

	return nBufferLength-len;
}

/*
 * Write a string to the logger
 *
 * 		type_channels		log type/channels
 * 								type:		log type, LT_xx
 * 								channels:	up to two channels to write log
 *		strFilename			source filename (may be NULL)
 *		nLine				source line
 *		strFunction			source function (may be NULL)
 *		pData				user data (may be NULL)
 * 		nLength				user data length, bytes
 *		strMessage			user message format
 *		args				VA arguments
 */
void CLogger::write(unsigned int type_channels, const char* strFilename, int nLine,
			const char* strFunction, const void* pData, int nLength,
			const char* strMessage, va_list args)
{
	unsigned int			lt = GET_TYPES(type_channels);
	int						index = type2i(lt);
	const logger_type_t*	pLogger;

	if ( lt == 0 || index < 0 || m_arAppender.size() < 1 )  {
		return;
	}

	pLogger = &m_arLogger[index];

	lock();
	if ( isEnabledType(pLogger, GET_CHANNEL0(type_channels)) ||
			isEnabledType(pLogger, GET_CHANNEL2(type_channels)) )
	{
		const char*		pFormat;
		char			strBuffer[LOGGER_BUFFER_MAX];
		size_t			nBufferLength;

		pFormat = strFilename ? pLogger->strFormat : LOGGER_FORMAT_DUMP;
		nBufferLength = doFormat(pLogger, pFormat, strMessage,
								strFilename, nLine, strFunction,
								strBuffer, sizeof(strBuffer), args);
		if ( pData != 0 && /* nLength > 0 &&*/ (sizeof(strBuffer)-1) > nBufferLength )  {
			nBufferLength += doFormatHex(pLogger, pData, nLength,
					&strBuffer[nBufferLength], sizeof(strBuffer)-nBufferLength);

			if ( nBufferLength < sizeof(strBuffer) )  {
				strBuffer[nBufferLength] = '\n';
				nBufferLength++;
				strBuffer[nBufferLength] = '\0';
			}
		}

		if ( nBufferLength > 0 )  {
			size_t		i, count = m_arAppender.size();

			for(i=0; i<count; i++)  {
				m_arAppender[i]->append(strBuffer, nBufferLength);
			}
		}
	}
	unlock();
}

result_t CLogger::getChannelData(int nChannel, void* pBuffer, size_t length)
{
	size_t		len;
	result_t	nresult = EINVAL;

	if ( nChannel >= I_DEBUG && nChannel < LOGGER_TYPES )  {
		len = MIN(length, sizeof(m_arLogger[nChannel].arChannel));
		lock();
		UNALIGNED_MEMCPY(pBuffer, &m_arLogger[nChannel].arChannel, len);
		unlock();

		nresult = ESUCCESS;
	}

	return nresult;
}


/*******************************************************************************
 * Global logger object
 */

static CLogger*	g_pLogger = 0;

/*
 * Logger bootstrap initialisation
 *
 *		bDefaultLogger		TRUE: create default console logger,
 *							FALSE: do not create
 *
 *	Note:
 *		Enable logging of the all channels and types
 */
void logger_init(boolean_t bDefaultLogger)
{
	if ( g_pLogger == 0 )  {
		appender_handle_t	hAppender;

		g_pLogger = new CLogger();
		if ( bDefaultLogger )  {
			hAppender = logger_addStdoutAppender();
			if ( hAppender == LOGGER_APPENDER_NULL )  {
				logger_syslog_impl("FAILED TO CREATE DEFAULT APPENDER, LOGGER MAY NOT BE AVAILABLE!\n");
			}
		}
	}
}

/*
 * Free logger resources and delete all appenders
 */
void logger_terminate()
{
	if ( g_pLogger )  {
		SAFE_DELETE(g_pLogger);
	}
}


/*
 * Enable logger types/channels
 *
 * 		type_channel		types/channels:
 * 								type:		enable log types LT_xxx (may be LT_ALL)
 * 								channel: 	log channels to enable L_xxx (may be L_ALL)
 */
void logger_enable(unsigned int type_channel)
{
	if ( g_pLogger )  {
		g_pLogger->enable(type_channel);
	}
}

/*
 * Disable logger types/channels
 *
 * 		type_channel		types/channels:
 * 								type:		disable log types LT_xxx (may be LT_ALL)
 * 								channel: 	log channel to disable L_xxx (may be L_ALL)
 */
void logger_disable(unsigned int type_channel)
{
	if ( g_pLogger )  {
		g_pLogger->disable(type_channel);
	}
}

/*
 * Check if a specified log type/channel is enabled
 *
 * 		type_channel		log type/channel to check (LT_xxx | L_xxx)
 *
 * Return:
 * 		TRUE: 	log is enabled
 * 		FALSE: 	log is disabled
 */
boolean_t logger_is_enabled(unsigned int type_channel)
{
	return g_pLogger ? g_pLogger->isEnabled(type_channel) : FALSE;
}

/*
 * Set output format for the specified log type messages
 *
 * 		type			log types LT_xxx (may be LT_ALL)
 * 		strFormat		format string (formats see above)
 */
void logger_set_format(unsigned int type, const char* strFormat)
{
	if ( g_pLogger )  {
		g_pLogger->setFormat(type, strFormat);
	}
}

/*
 * Set output format for the timestamp
 *
 * 		type			log types LT_xxx (may be LT_ALL)
 * 		strFormat		format string
 */
void logger_set_format_time(unsigned int type, const char* strFormat)
{
	if ( g_pLogger )  {
		g_pLogger->setFormatTime(type, strFormat);
	}
}

/*
 * Write log message
 *
 * 		type_channels		log type/channels
 * 								type:		log type, LT_xx
 * 								channels:	up to two channels to write log
 *		strFilename			source filename (may be NULL)
 *		line				source line
 *		strFunction			source function (may be NULL)
 *		pData				user data (may be NULL)
 *		length				user data length, bytes
 *		strMessage			user message format
 */
void logger_write(unsigned int type_channel, const char* strFilename, int line,
		const char* strFunction, const void* pData, int length, const char* strMessage, ...)
{
   if ( g_pLogger )  {
    	va_list     	args;

    	va_start(args, strMessage);

    	g_pLogger->write(type_channel, strFilename, line, strFunction, pData,
    						length, strMessage, args);
    	va_end(args);
    }
}

/*
 * Insert an appender to the logger
 *
 * 		pAppender		appender object (uninitialised)
 *
 * Return: appender handler or LOGGER_APPENDER_NULL
 */
appender_handle_t logger_insert_appender_impl(CAppender* pAppender)
{
	appender_handle_t	hAppender = LOGGER_APPENDER_NULL;

	if ( g_pLogger )  {
		hAppender = g_pLogger->insertAppender(pAppender);
	}

	return hAppender;
}

result_t logger_remove_appender(appender_handle_t hAppender)
{
	result_t	nresult = ENOENT;

	if ( g_pLogger )  {
		nresult = g_pLogger->removeAppender(hAppender);
	}

	return nresult;
}

result_t logger_get_channel(int nIndex, void* pBuffer, size_t length)
{
	result_t nresult = EFAULT;

	if ( g_pLogger ) {
		nresult = g_pLogger->getChannelData(nIndex, pBuffer, length);
	}

	return nresult;
}
