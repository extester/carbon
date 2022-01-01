/*
 *  Shell library
 *  Logger
 *
 *  Copyright (c) 2013-2021 Softland. All rights reserved.
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
 *  Revision 2.1, 25.12.2019 19:18:20
 *  	Added LT_TRACE log type support.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <syslog.h>

#include "shell/defines.h"
#include "shell/tstring.h"
#include "shell/tstdio.h"
#include "shell/unaligned.h"
#include "shell/logger/appender.h"
#include "shell/logger/appender_stdout.h"
#include "shell/logger/logger.h"

/*
 * Log output format:
 *
 * 		%n				carriage return
 * 		%F				source file
 * 		%N				source line
 * 		%P				source function
 * 		%T				timestamp (see separate time format)
 * 		%s				user message
 * 		%l				log type: 'TRC', 'INF', 'WRN', 'ERR', 'DBG'
 * 		%p				process ID
 * 		%t				pthread thread ID (available if !LOGGER_SINGLE_THREAD)
 */

#define LOGGER_FORMAT_DEFAULT			"%T [%l] %P(%N): %s"
#define LOGGER_FORMAT_TIME_DEFAULT		"%d.%m.%y %H:%M:%S"
#define LOGGER_FORMAT_DUMP				"%T: %s"

/*******************************************************************************
 * CLogger class
 */

CLogger::CLogger(boolean_t bDefaultAppender)
{
	static const char* arStrType[] = { "DBG", "ERR", "WRN", "INF", "TRC" };

	_tbzero_object(m_arAppender);
	_tbzero_object(m_arType);

#if !LOGGER_SINGLE_THREAD
	pthread_mutex_init(&m_lock, nullptr);
#endif /* !LOGGER_SINGLE_THREAD */

	for(size_t i=1; i<=LOGGER_TYPES; i++)  {
		_tmemset(&m_arType[i].arChannel, 0xff, sizeof(m_arType[i].arChannel));
		setFormat(LT_ALL, LOGGER_FORMAT_DEFAULT);
		setFormatTime(LT_ALL, LOGGER_FORMAT_TIME_DEFAULT);
		CLogger::copyString(m_arType[i].strType, arStrType[i-1], sizeof(m_arType[i].strType));
	}

	if ( bDefaultAppender )  {
		insertAppender(new CAppenderStdout);
	}
}

CLogger::~CLogger() noexcept
{
	lock();
	for (auto& pAppender : m_arAppender) {
		if ( pAppender ) {
			pAppender->terminate();
			delete pAppender;
		}
		else {
			break;
		}
	}
	unlock();

#if !LOGGER_SINGLE_THREAD
	pthread_mutex_destroy(&m_lock);
#endif /* !LOGGER_SINGLE_THREAD */
}

/*
 * Enable logger type/channel
 *
 * 		typeChannel		enabling log types/channels
 * 							type:		enable log types LT_xxx (may be LT_ALL)
 * 							channel: 	log channels to enable L_xxx (may be L_ALL)
 */
void CLogger::enable(unsigned int typeChannel)
{
	const unsigned int	ch = _LOGGER_CHANNEL0(typeChannel);
	const unsigned int 	type = _LOGGER_TYPE(typeChannel);
	const unsigned int	exc_index = L_ALWAYS_DISABLED/8;
	const unsigned int	exc_bit = 1U << (L_ALWAYS_DISABLED&7);

	auto Enable = [] (logger_type_t* pType, unsigned int ch) {
		if ( ch != 0 ) {
			const unsigned int index = ch/8;
			const unsigned int bit = 1U << (ch&7);
			pType->arChannel[index] |= bit;
		}
		else {
			_tmemset(&pType->arChannel, 0xff, sizeof(pType->arChannel));
		}

		pType->arChannel[exc_index] &= ~exc_bit;
	};

	lock();
	if ( type != 0 )  {
		Enable(&m_arType[type], ch);
	}
	else {
		for(size_t i=1; i<=LOGGER_TYPES; i++) {
			Enable(&m_arType[i], ch);
		}
	}
	unlock();
}

/*
 * Disable logger type/channel
 *
 * 		typeChannel		disabling log types/channels
 * 							type:		enable log types LT_xxx (may be LT_ALL)
 * 							channel: 	log channels to enable L_xxx (may be L_ALL)
 */
void CLogger::disable(unsigned int typeChannel)
{
	const unsigned int	ch = _LOGGER_CHANNEL0(typeChannel);
	const unsigned int 	type = _LOGGER_TYPE(typeChannel);
	const unsigned int	exc_index = L_ALWAYS_ENABLED/8;
	const unsigned int	exc_bit = 1U << (L_ALWAYS_ENABLED&7);

	auto Disable = [] (logger_type_t* pType, unsigned int ch) {
		if ( ch != 0 ) {
			const unsigned int index = ch/8;
			const unsigned int bit = 1U << (ch&7);
			pType->arChannel[index] &= ~bit;
		}
		else {
			_tbzero_object(pType->arChannel);
		}

		pType->arChannel[exc_index] |= exc_bit;
	};

	lock();
	if ( type != 0 )  {
		Disable(&m_arType[type], ch);
	}
	else {
		for(size_t i=1; i<=LOGGER_TYPES; i++) {
			Disable(&m_arType[i], ch);
		}
	}
	unlock();
}

/*
 * Check if a specified log type/channel is enabled
 *
 * 		typeChannel		log type/channel to check (LT_xxx | L_xxx)
 *
 * Return:
 * 		true			log is enabled
 * 		false			log is disabled
 */
boolean_t CLogger::isEnabled(unsigned int typeChannel)
{
	const unsigned int	ch = _LOGGER_CHANNEL0(typeChannel);
	const unsigned int	type = _LOGGER_TYPE(typeChannel);
	boolean_t			bEnabled = false;

	if ( ch != 0 && (type > 0 && type <= LOGGER_TYPES) )  {
		unsigned int 	index, bit;

		index = ch/8;
		bit = (unsigned int)1 << (ch&7);

		lock();
		bEnabled = (m_arType[type].arChannel[index]&bit) != 0;
		unlock();
	}

	return bEnabled;
}

/*
 * Set output format for the specified log type messages
 *
 * 		type			log types LT_xxx (may be LT_ALL)
 * 		strFormat		format string (formats see above)
 */
void CLogger::setFormat(unsigned int type, const char* strFormat)
{
	const unsigned int	rtype = _LOGGER_TYPE(type);

	if ( rtype >= 0 && rtype <= LOGGER_TYPES )  {
		lock();
		if ( rtype > 0 )  {
			CLogger::copyString(m_arType[rtype].strFormat, strFormat,
					   sizeof(m_arType[rtype].strFormat));
		}
		else {
			for(size_t i=1; i<=LOGGER_TYPES; i++)  {
				CLogger::copyString(m_arType[i].strFormat, strFormat,
									sizeof(m_arType[i].strFormat));
			}
		}
		unlock();
	}
}

/*
 * Set output format for the timestamp
 *
 * 		type			log types LT_xxx (may be LT_ALL)
 * 		strFormat		format string
 */
void CLogger::setFormatTime(unsigned int type, const char* strFormat)
{
	const unsigned int	rtype = _LOGGER_TYPE(type);

	if ( rtype >= 0 && rtype <= LOGGER_TYPES )  {
		lock();
		if ( rtype > 0 )  {
			CLogger::copyString(m_arType[rtype].strFormatTime, strFormat,
								sizeof(m_arType[rtype].strFormatTime));
		}
		else {
			for(size_t i=1; i<=LOGGER_TYPES; i++)  {
				CLogger::copyString(m_arType[i].strFormatTime, strFormat,
									sizeof(m_arType[i].strFormatTime));
			}
		}
		unlock();
	}
}

/*
 * Check if a specified channel is enabled
 *
 * 		pLogger		logger channel pointer
 * 		ch			channel bitmap to check
 *
 * Return:
 * 		true		log is enabled
 * 		false		log is disabled
 */
boolean_t CLogger::isEnabledChannel(const logger_type_t* pLogger, unsigned int ch) const
{
	boolean_t	bEnabled = false;

	if ( ch != 0 )  {
		const unsigned int	index = ch/8;
		const unsigned int	bit = 1U << (ch&7);

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
 * 		typeChannels		log type/channels
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
void CLogger::write(unsigned int typeChannels, const char* strFilename, int nLine,
					const char* strFunction, const void* pData, int nLength,
					const char* strMessage, ...)
{
	va_list     	args;

	va_start(args, strMessage);
	writev(typeChannels, strFilename, nLine, strFunction, pData, nLength, strMessage, args);
	va_end(args);
}

void CLogger::writev(unsigned int typeChannels, const char* strFilename, int nLine,
					const char* strFunction, const void* pData, int nLength,
					 const char* strMessage, va_list args)
{
	const unsigned int		type = _LOGGER_TYPE(typeChannels);
	const logger_type_t*	pLogger;

	if ( (type < 1 || type > LOGGER_TYPES) || m_arAppender[0] == nullptr )  {
		return;
	}

	pLogger = &m_arType[type];

	lock();
	if ( isEnabledChannel(pLogger, _LOGGER_CHANNEL0(typeChannels)) ||
			isEnabledChannel(pLogger, _LOGGER_CHANNEL2(typeChannels)) )
	{
		const char*		pFormat;
		char			strBuffer[LOGGER_BUFFER_MAX];
		size_t			nBufferLength;

		pFormat = strFilename ? pLogger->strFormat : LOGGER_FORMAT_DUMP;
		nBufferLength = doFormat(pLogger, pFormat, strMessage,
								 strFilename, nLine, strFunction,
								 strBuffer, sizeof(strBuffer), args);

		if ( pData != 0 && /* nLength > 0 &&*/ (sizeof(strBuffer)-2) > nBufferLength )  {
			nBufferLength += doFormatHex(pLogger, pData, nLength,
					&strBuffer[nBufferLength], sizeof(strBuffer)-nBufferLength);

			if ( nBufferLength < (sizeof(strBuffer)-2) )  {
				strBuffer[nBufferLength] = '\n';
				nBufferLength++;
				strBuffer[nBufferLength] = '\0';
			}
		}
		else {
			if ( nBufferLength > 0 && nBufferLength >= (sizeof(strBuffer)-2) )  {
				strBuffer[nBufferLength-2] = '\n';
				strBuffer[nBufferLength-1] = '\0';
			}
		}

		if ( nBufferLength > 0 )  {
			for(auto& pAppender : m_arAppender)  {
				if ( pAppender != nullptr ) {
					pAppender->append(strBuffer, nBufferLength);
				}
				else {
					break;
				}
			}
		}
	}
	unlock();
}

/*
 * Get logger type enable/disable bitmap data
 *
 * 		type			logger type, LT_xxx
 * 		pBuffer			buffer for data
 * 		nLength			buffer length, bytes
 *
 * Return: ESUCCESS, EINVAL
 */
result_t CLogger::getEnableData(unsigned int type, void* pBuffer, size_t nLength)
{
	const unsigned int	rtype = _LOGGER_TYPE(type);
	size_t				len;
	result_t			nresult = EINVAL;

	if ( rtype > 0 && rtype <= LOGGER_TYPES )  {
		len = sh_min(nLength, sizeof(m_arType[rtype].arChannel));

		lock();
		UNALIGNED_MEMCPY(pBuffer, &m_arType[rtype].arChannel, len);
		unlock();

		nresult = ESUCCESS;
	}

	return nresult;
}

/*
 * Add appender to the logger
 *
 * 		pAppender		appender object
 *
 * Return: ESUCCESS, EINVAL, ENOMEM
 */
result_t CLogger::insertAppender(CAppender* pAppender)
{
	result_t	nresult = ENOMEM;

	if ( dynamic_cast<CAppender*>(pAppender) == nullptr )  {
		CLogger::syslog("[shell_logger] invalid appender instance\n");
		return EINVAL;
	}

	nresult = pAppender->init();
	if ( nresult == ESUCCESS ) {
		nresult = ENOENT;

		for(size_t i=0; i<ARRAY_SIZE(m_arAppender); i++) {
			if ( m_arAppender[i] == nullptr )  {
				m_arAppender[i] = pAppender;
				nresult = ESUCCESS;
				break;
			}
		}

		if ( nresult != ESUCCESS )  {
			pAppender->terminate();
		}
	}

	if ( nresult != ESUCCESS )  {
		CLogger::syslog("[shell_logger] too many appenders in the logger\n");
	}

	return nresult;
}

/*******************************************************************************
 * Static helper functions
 */

/*
 * Write message to syslog
 *
 * 		strMessage		message to write
 */
void CLogger::syslog(const char* strMessage, ...)
{
	va_list     		args;
	static boolean_t	bInitialised = false;

	if ( !bInitialised )  {
		::openlog(NULL, SYSLOG_OPTIONS, SYSLOG_FACILITY);
		bInitialised = true;
	}

	va_start(args, strMessage);
	vsyslog(LOG_ERR, strMessage, args);
	va_end(args);
}

/*
 * Copy string
 *
 *		strDst				destination address
 * 		strSrc				source string
 * 		nMaxLength			destination buffer size
 */
void CLogger::copyString(char* strDst, const char* strSrc, size_t nMaxLength)
{
	size_t	nLength = _tstrlen(strSrc);

	nLength = sh_min(nLength, (nMaxLength-1));
	UNALIGNED_MEMCPY(strDst, strSrc, nLength);
	strDst[nLength] = '\0';
}

/*
 * Append buffer to a file
 *
 *      strFilename			full filename
 *      pBuffer				buffer to write
 *      nLength				buffer length, bytes
 *
 * Return: ESUCCESS, ...
 *
 * Note: The logging function couldn't be used in the function
 * Note: use O_APPEND for atomically write
 */
result_t CLogger::appendFile(const char* strFilename, const void* pBuffer, size_t nLength)
{
	int         handle;
	result_t	nresult = ESUCCESS;
	ssize_t     count;

	handle = ::open(strFilename, O_CREAT|O_WRONLY|O_APPEND, (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
	if ( handle >= 0 )  {
		count = ::write(handle, pBuffer, nLength);
		if ( count < (ssize_t)nLength )  {
			nresult = count < 0 ? errno : EIO;
		}

		::close(handle);
	}
	else  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Global logger instance
 */
CLogger g_logger(true);
