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
 */

#ifndef __SHELL_LOGGER_BASE_H_INCLUDED__
#define __SHELL_LOGGER_BASE_H_INCLUDED__

#include <vector>

#if !LOGGER_SINGLE_THREAD
#include <pthread.h>
#endif /* !LOGGER_SINGLE_THREAD */

#include "shell/config.h"
#include "shell/types.h"
#include "shell/logger.h"


/*
 * Use syslog parameters
 */
#define SYSLOG_OPTIONS			(LOG_CONS|LOG_NDELAY)
#define SYSLOG_FACILITY			LOG_USER
#define SYSLOG_PRIORITY			LOG_ERR

/*
 * Logger type/channel parameter
 *
 * 		tttttttt -------- --nnyyyy yyyyyyxx xxxxxxxx
 *
 * 		xx			first channel number
 * 		yy			second channel number
 * 		--			unused
 * 		tt			channel type(s) (bitmap)
 */

#define CHANNEL_MASK					(LOGGER_CHANNEL_MAX-1)

#define GET_CHANNEL0(__types_channel)	((__types_channel)&CHANNEL_MASK)
#define GET_CHANNEL2(__types_channel)	(((__types_channel)>>LOGGER_CHANNEL_BIT_MAX)&CHANNEL_MASK)
#define GET_TYPES(__types)				((__types)&(~(CHANNEL_MASK|(CHANNEL_MASK<<LOGGER_CHANNEL_BIT_MAX))))


/*
 * Log output format:
 *
 * 		%n				carriage return
 * 		%F				source file
 * 		%N				source line
 * 		%P				source function
 * 		%T				timestamp (see separate time format)
 * 		%s				user message
 * 		%l				log type: 'INF', 'WRN', 'ERR', 'DBG'
 * 		%p				process ID
 * 		%t				pthread thread ID (available if !LOGGER_SINGLE_THREAD)
 */

#define LOGGER_FORMAT_MAX				128

#define LOGGER_FORMAT_DEFAULT			"%T [%l] %P(%N): %s"
#define LOGGER_FORMAT_TIME_DEFAULT		"%d.%m.%y %H:%M:%S"
#define LOGGER_FORMAT_DUMP				"%T: %s"

#define LOGGER_BUFFER_MAX				CARBON_LOGGER_BUFFER_LENGTH


/*******************************************************************************
 * Appenders base class
 */

class CAppender
{
	protected:
		CAppender() {}
	public:
		virtual ~CAppender() {}

	public:
		appender_handle_t getHandle() const { return (appender_handle_t)this; }
		static CAppender* fromHandle(appender_handle_t hAppender) {
			return reinterpret_cast<CAppender*>(hAppender);
		}

	public:
		virtual result_t init() = 0;
		virtual void terminate() = 0;
		virtual result_t append(const void* pData, size_t nLength) = 0;
};


/*******************************************************************************
 * Logger class
 */

/* Single logger type data */
typedef struct logger_type
{
	char			strType[4];							/* 'INF', 'WRN', 'ERR', 'DBG' */
	uint8_t			arChannel[LOGGER_CHANNEL_MAX/8];	/* Channel enabled bitmap */
	char			strFormat[LOGGER_FORMAT_MAX];		/* Logger line format */
	char			strFormatTime[LOGGER_FORMAT_MAX];	/* Logger timestamp format */
} logger_type_t;

/* Logger type indexes */
#define I_DEBUG				0			/* Debug 'DBG' */
#define I_ERROR				1			/* Error 'ERR' */
#define I_WARNING			2			/* Warning 'WRN' */
#define I_INFO				3			/* Information 'INF' */

#define LOGGER_TYPES		4

class CLogger
{
	protected:
		std::vector<CAppender*>		m_arAppender;
		logger_type_t				m_arLogger[LOGGER_TYPES];
#if !LOGGER_SINGLE_THREAD
		pthread_mutex_t				m_lock;
#endif /* !LOGGER_SINGLE_THREAD */

	public:
		CLogger();
		virtual ~CLogger();

	public:
		appender_handle_t insertAppender(CAppender* pAppender);
		result_t removeAppender(appender_handle_t hAppender);

		void setFormat(unsigned int type, const char* strFormat);
		void setFormatTime(unsigned int type, const char* strFormat);

		void enable(unsigned int type_channel);
		void disable(unsigned int type_channel);
		boolean_t isEnabled(unsigned int type_channel);

		void write(unsigned int type_channels, const char* strFilename, int nLine,
					const char* strFunction, const void* pData, int nLength,
					const char* strMessage, va_list args);

		result_t getChannelData(int nChannel, void* pBuffer, size_t length);

	protected:
		void lock() {
#if !LOGGER_SINGLE_THREAD
			pthread_mutex_lock(&m_lock);
#endif /* !LOGGER_SINGLE_THREAD */
		}

		void unlock() {
#if !LOGGER_SINGLE_THREAD
			pthread_mutex_unlock(&m_lock);
#endif /* !LOGGER_SINGLE_THREAD */
		}

		unsigned int i2type(unsigned int index) const;
		int type2i(unsigned int type) const;
		boolean_t isEnabledType(const logger_type_t* pLogger, unsigned int channel) const;

		size_t doFormat(const logger_type_t* pLogger, const char* strLoggerFormat,
				const char* strMessage, const char* strFilename, int nLine,
				const char* strFunction, char* strBuffer, size_t nBufferLength,
				va_list args) const;

		size_t doFormatHex(const logger_type_t* pLogger, const void* pData, int nLength,
								char* strBuffer, size_t nBufferLength) const;
};

/*******************************************************************************/

/*
 * Utility functions
 */
extern void logger_syslog_impl(const char* strMessage, ...);
extern void logger_copy_string_impl(char* dst, const char* src, size_t n);
extern result_t logger_append_file(const char* filename, const void* buf, size_t length);

extern appender_handle_t logger_insert_appender_impl(CAppender* pAppender);

#endif /* __SHELL_LOGGER_BASE_H_INCLUDED__ */
