/*
 *  Shell library
 *  Logger (UNIX)
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 26.01.2015 12:31:17
 *      Initial revision.
 *
 *  Revision 1.1, 25.02.2015 13:32:26
 *  	Added CAppenderTcpServer appender.
 *
 *  Revision 1.2, 04.04.2015 21:35:49
 *  	Added L_ALWAYS_DISABLED log channel.
 */

#ifndef __SHELL_LOGGER_H_INCLUDED__
#define __SHELL_LOGGER_H_INCLUDED__

#include <stdarg.h>

#include "shell/types.h"


/*
 * Output log types
 */

#define LT_ALL					0x00000000	/* All types selected */
#define LT_DEBUG 				0x01000000	/* 'DEBUG' log type */
#define LT_ERROR				0x02000000	/* 'ERROR' log type */
#define LT_WARNING				0x04000000	/* 'WARNING' log type */
#define LT_INFO					0x08000000	/* 'INFO' log type */

/*
 * Maximum supported log channels, 10 bits => up to 1024
 */
#define LOGGER_CHANNEL_BIT_MAX	10
#define LOGGER_CHANNEL_MAX		(1<<LOGGER_CHANNEL_BIT_MAX)

/*
 * Output log channels
 *
 * 		Channels 0..255 are reserved for the system/library purpose
 */

#define L_ALL					0			/* All channels selected */
#define L_ALWAYS_ENABLED		1			/* Always enabled channel */
#define L_ALWAYS_DISABLED		2			/* Always disabled channel */
#define L_GEN					3			/* Default 'GENERIC' channel */

#define L_SHELL					32			/* First shell library available channel */

#define L_GEN_FL				(L_SHELL+0)
#define L_SOCKET				(L_SHELL+1)
#define L_SOCKET_FL				(L_SHELL+2)
#define L_FILE					(L_SHELL+3)
#define L_FILE_FL				(L_SHELL+4)
#define L_MIO					(L_SHELL+5)
#define L_NET_FL				(L_SHELL+6)
#define L_ICMP					(L_SHELL+7)
#define L_ICMP_FL				(L_SHELL+8)
#define L_EV_TRACE_TIMER		(L_SHELL+9)
#define L_EV_TRACE_EVENT		(L_SHELL+10)
#define L_BOOT					(L_SHELL+11)
#define L_DB					(L_SHELL+12)
#define L_DB_SQL_TRACE			(L_SHELL+13)

#define L_SHELL_USER			(L_DB_SQL_TRACE+1)


/*
 * Logger appender handle
 */
typedef unsigned int*			appender_handle_t;
#define LOGGER_APPENDER_NULL	((appender_handle_t)0)

/*
 * Logger module C/CPP API functions
 */

/*
 * Log message
 *
 * log_info(), log_warning(), log_error(), log_debug()
 * 		got a single log channel and write a log message
 *
 * log_info2(), log_warning2(), log_error2(), log_debug2()
 * 		got the two log channels and write a log message
 */
#define log_info(__channel, __message, ...)	\
	logger_write(LT_INFO|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				0, 0, __message, ##__VA_ARGS__)

#define log_info2(__channel, __channel2, __message, ...)	\
	logger_write(LT_INFO|(__channel)|((__channel2)<<LOGGER_CHANNEL_BIT_MAX), \
				__FILE__, __LINE__, __FUNCTION__, 0, 0, __message, ##__VA_ARGS__)

#define log_warning(__channel, __message, ...)	\
	logger_write(LT_WARNING|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				0, 0, __message, ##__VA_ARGS__)

#define log_warning2(__channel, __channel2, __message, ...)	\
	logger_write(LT_WARNING|(__channel)|((__channel2)<<LOGGER_CHANNEL_BIT_MAX), \
				__FILE__, __LINE__, __FUNCTION__, 0, 0, __message, ##__VA_ARGS__)

#define log_error(__channel, __message, ...)	\
	logger_write(LT_ERROR|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				0, 0, __message, ##__VA_ARGS__)

#define log_error2(__channel, __channel2, __message, ...)	\
	logger_write(LT_ERROR|(__channel)|((__channel2)<<LOGGER_CHANNEL_BIT_MAX), \
				__FILE__, __LINE__, __FUNCTION__, 0, 0, __message, ##__VA_ARGS__)

#define log_debug(__channel, __message, ...)	\
	logger_write(LT_DEBUG|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				0, 0, __message, ##__VA_ARGS__)

#define log_debug2(__channel, __channel2, __message, ...)	\
	logger_write(LT_DEBUG|(__channel)|((__channel2)<<LOGGER_CHANNEL_BIT_MAX), \
				__FILE__, __LINE__, __FUNCTION__, 0, 0, __message, ##__VA_ARGS__)

/*
 * Log message and hex dump:
 * 		<message><dump>
 *
 * log_debug_bin(), log_debug_bin2()
 * 		got one/two log channels and write string/hex data of the debug type
 */
#define log_debug_bin(__channel, __data, __length, __message, ...)	\
	logger_write(LT_DEBUG|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				__data, __length, __message, ##__VA_ARGS__)

#define log_debug_bin2(__channel, __channel2, __data, __length, __message, ...)	\
	logger_write(LT_DEBUG|(__channel)|((__channel2)<<LOGGER_CHANNEL_BIT_MAX), \
				__FILE__, __LINE__, __FUNCTION__, __data, __length, __message, ##__VA_ARGS__)

/*
 * Log message with the predefined format LOGGER_FORMAT_DUMP (see logger.cpp),
 * log type LT_DEBUG, message can't be disabled
 *
 * log_dump()
 * 		write raw message of the debug type
 *
 * log_dump_bin()
 * 		write raw message accompanied by memory HEX dump
 */
#define log_dump(__message, ...)	\
	logger_write(LT_DEBUG|L_ALWAYS_ENABLED, NULL, 0, NULL, NULL, 0, __message, ##__VA_ARGS__)

#define log_dump_bin(__data, __length, __message, ...)	\
	logger_write(LT_DEBUG|L_ALWAYS_ENABLED, NULL, 0, NULL, \
				__data, __length, __message, ##__VA_ARGS__)

extern void logger_init(boolean_t bDefaultLogger);
extern void logger_terminate();

extern void logger_enable(unsigned int type_channel);
extern void logger_disable(unsigned int type_channel);
extern boolean_t logger_is_enabled(unsigned int type_channel);

extern void logger_set_format(unsigned int type, const char* strFormat);
extern void logger_set_format_time(unsigned int type, const char* strFormat);

extern void logger_write(unsigned int type_channel, const char* strFilename, int line,
						 const char* strFunction, const void* pData, int length, const char* strMessage, ...);

extern result_t logger_get_channel(int nIndex, void* pBuffer, size_t length);
extern result_t logger_remove_appender(appender_handle_t hAppender);

/*
 * Available appenders
 */
extern appender_handle_t logger_addStdoutAppender();
extern appender_handle_t logger_addFileAppender(const char* strFilename,
									size_t nFileCountMax, size_t nFileSizeMax);
extern appender_handle_t logger_addSyslogAppender(const char* strIdent);
#if !LOGGER_SINGLE_THREAD
extern appender_handle_t logger_addTcpServerAppender(uint16_t nPort);
#endif /* !LOGGER_SINGLE_THREAD */
extern appender_handle_t logger_addPickupAppender(const char* strFilename);

extern result_t logger_getPickupAppenderBlock(appender_handle_t hAppender,
									void* pBuffer, size_t* pSize);
extern void logger_getPickupAppenderBlockConfirm(appender_handle_t hAppender,
									size_t size);
extern void logger_getPickupAppenderBlockUndo(appender_handle_t hAppender,
									size_t size);

#endif /* __SHELL_LOGGER_H_INCLUDED__ */
