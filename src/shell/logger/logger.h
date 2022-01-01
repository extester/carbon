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
 *  Revision 1.0, 26.01.2015 16:28:41
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
 *  Revision 2.0, 25.08.2015 13:10:41
 *  	Separate appenders to the modules.
 *
 *  Revision 2.1, 25.12.2019 19:17:47
 *  	Added LT_TRACE log type support.
 */

#ifndef __SHELL_LOGGER_H_INCLUDED__
#define __SHELL_LOGGER_H_INCLUDED__

#include <stdarg.h>

#include "shell/config.h"
#include "shell/types.h"
#include "shell/error.h"

#if !LOGGER_SINGLE_THREAD
#include <pthread.h>
#endif /* !LOGGER_SINGLE_THREAD */

/* Use syslog parameters */
#define SYSLOG_OPTIONS					(LOG_CONS|LOG_NDELAY)
#define SYSLOG_FACILITY					LOG_USER
#define SYSLOG_PRIORITY					LOG_ERR

/*
 * Logger type/channel parameter
 *
 * 		-------- ttttyyyy yyyyyyxx xxxxxxxx
 *
 * 		xx			first channel number
 * 		yy			second channel number
 * 		tt			channel type index (0-all)
 * 		--			unused
 */

/* Maximum supported log channels, 10 bits => up to 1024 */
#define LOGGER_CHANNEL_BITS				10U
#define LOGGER_CHANNEL_MAX				(1U<<LOGGER_CHANNEL_BITS)

#define CHANNEL_MASK					(LOGGER_CHANNEL_MAX-1)
#define TYPE_MASK						0xFU

#define _LOGGER_CHANNEL0(__typeChannel)	((__typeChannel)&CHANNEL_MASK)
#define _LOGGER_CHANNEL2(__typeChannel)	(((__typeChannel)>>LOGGER_CHANNEL_BITS)&CHANNEL_MASK)
#define _LOGGER_TYPE(__type) 			(((__type)>>(LOGGER_CHANNEL_BITS*2))&TYPE_MASK)

/*
 * Logger types
 */

#define LT_ALL					(0<<(LOGGER_CHANNEL_BITS*2))	/* All types selected */
#define LT_DEBUG 				(1<<(LOGGER_CHANNEL_BITS*2))	/* 'DEBUG' log type */
#define LT_ERROR				(2<<(LOGGER_CHANNEL_BITS*2))	/* 'ERROR' log type */
#define LT_WARNING				(3<<(LOGGER_CHANNEL_BITS*2))	/* 'WARNING' log type */
#define LT_INFO					(4<<(LOGGER_CHANNEL_BITS*2))	/* 'INFO' log type */
#define LT_TRACE				(5<<(LOGGER_CHANNEL_BITS*2))	/* 'TRACE' log type */

#define LOGGER_TYPES			5


/*
 * Logger predefined channels
 */

#define L_ALL					0U			/* All channels selected */
#define L_ALWAYS_ENABLED		1U			/* Always enabled channel */
#define L_ALWAYS_DISABLED		2U			/* Always disabled channel */
#define L_GEN					3U			/* Default 'GENERIC' channel */

#define L_SHELL					32U			/* First shell library available channel */

#define LOGGER_APPENDER_MAX		16			/* Maximum appenders per logger */
#define LOGGER_FORMAT_MAX		128
#define LOGGER_BUFFER_MAX		CARBON_LOGGER_BUFFER_LENGTH

class CAppender;

/*
 * Logger base class
 */
class CLogger
{
	protected:
		struct logger_type_t {
			char			strType[LOGGER_TYPES+1];			/* '', 'DBG', ... 'TRC' */
			uint8_t			arChannel[LOGGER_CHANNEL_MAX/8];	/* Channel enabled bitmap */
			char			strFormat[LOGGER_FORMAT_MAX];		/* Logger line format */
			char			strFormatTime[LOGGER_FORMAT_MAX];	/* Logger timestamp format */
		};

	protected:
		CAppender*					m_arAppender[LOGGER_APPENDER_MAX];
		logger_type_t				m_arType[LOGGER_TYPES+1];

#if !LOGGER_SINGLE_THREAD
		mutable pthread_mutex_t		m_lock;
#endif /* !LOGGER_SINGLE_THREAD */

	public:
		explicit CLogger(boolean_t bDefaultAppender);
		virtual ~CLogger() noexcept;

	protected:
		void lock() const {
#if !LOGGER_SINGLE_THREAD
			pthread_mutex_lock(&m_lock);
#endif /* !LOGGER_SINGLE_THREAD */
		}

		void unlock() const {
#if !LOGGER_SINGLE_THREAD
			pthread_mutex_unlock(&m_lock);
#endif /* !LOGGER_SINGLE_THREAD */
		}

		boolean_t isEnabledChannel(const logger_type_t* pLogger, unsigned int ch) const;
		size_t doFormat(const logger_type_t* pLogger, const char* strLoggerFormat,
					 	const char* strMessage, const char* strFilename, int nLine,
					 	const char* strFunction, char* strBuffer, size_t nBufferLength,
					 	va_list args) const;
		size_t doFormatHex(const logger_type_t* pLogger, const void* pData, int nLength,
						char* strBuffer, size_t nBufferLength) const;

	public:
		/*
		 * API functions
		 */
		virtual void enable(unsigned int typeChannel);
		virtual void disable(unsigned int typeChannel);
		virtual boolean_t isEnabled(unsigned int typeChannel);

		void setFormat(unsigned int type, const char* strFormat);
		void setFormatTime(unsigned int type, const char* strFormat);

		virtual void write(unsigned int typeChannels, const char* strFilename,
					 	int nLine, const char* strFunction, const void* pData,
					 	int nLength, const char* strMessage, ...);

		virtual void writev(unsigned int typeChannels, const char* strFilename,
					   int nLine, const char* strFunction, const void* pData,
					   int nLength, const char* strMessage, va_list args);

		result_t getEnableData(unsigned int type, void* pBuffer, size_t nLength);
		result_t insertAppender(CAppender* pAppender);

	public:
		/*
		 * Static helper functions
		 */
		static void syslog(const char* strMessage, ...);
		static void copyString(char* strDst, const char* strSrc, size_t nMaxLength);
		static result_t appendFile(const char* strFilename, const void* pBuffer, size_t nLength);
};

extern CLogger g_logger;

#endif /* __SHELL_LOGGER_H_INCLUDED__ */
