/*
 *  Shell library
 *  Default logger macros
 *
 *  Copyright (c) 2021 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 21.01.2021 19:51:45
 *      Initial revision.
 */

#ifndef __SHELL_LOGGER_COMPAT_H_INCLUDED__
#define __SHELL_LOGGER_COMPAT_H_INCLUDED__

#include "shell/logger/logger.h"

/*
 * Shell library logger channels
 */
#define L_SHELL					32U			/* First shell library available channel */

#define L_SOCKET				(L_SHELL+0)
#define L_FILE					(L_SHELL+1)
#define L_MIO					(L_SHELL+2)
#define L_NET					(L_SHELL+3)
#define L_ICMP					(L_SHELL+4)
#define L_BOOT					(L_SHELL+5)
#define L_DB					(L_SHELL+6)
#define L_TIMER					(L_SHELL+7)
#define L_EVENT					(L_SHELL+8)

#define L_SHELL_USER			(L_EVENT+1)

/*
 * Log message
 *
 * log_info(), log_warning(), log_error(), log_debug(), log_trace()
 * 		got a single log channel and write a log message
 *
 * log_info2(), log_warning2(), log_error2(), log_debug2(), log_trace2()
 * 		got the two log channels and write a log message
 */
#define log_info(__channel, __message, ...)	\
	g_logger.write(LT_INFO|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				0, 0, __message, ##__VA_ARGS__)

#define log_info2(__channel, __channel2, __message, ...)	\
	g_logger.write(LT_INFO|(__channel)|((__channel2)<<LOGGER_CHANNEL_BITS), \
				__FILE__, __LINE__, __FUNCTION__, 0, 0, __message, ##__VA_ARGS__)

#define log_warning(__channel, __message, ...)	\
	g_logger.write(LT_WARNING|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				0, 0, __message, ##__VA_ARGS__)

#define log_warning2(__channel, __channel2, __message, ...)	\
	g_logger.write(LT_WARNING|(__channel)|((__channel2)<<LOGGER_CHANNEL_BITS), \
				__FILE__, __LINE__, __FUNCTION__, 0, 0, __message, ##__VA_ARGS__)

#define log_error(__channel, __message, ...)	\
	g_logger.write(LT_ERROR|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				0, 0, __message, ##__VA_ARGS__)

#define log_error2(__channel, __channel2, __message, ...)	\
	g_logger.write(LT_ERROR|(__channel)|((__channel2)<<LOGGER_CHANNEL_BITS), \
				__FILE__, __LINE__, __FUNCTION__, 0, 0, __message, ##__VA_ARGS__)

#define log_debug(__channel, __message, ...)	\
	g_logger.write(LT_DEBUG|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				0, 0, __message, ##__VA_ARGS__)

#define log_debug2(__channel, __channel2, __message, ...)	\
	g_logger.write(LT_DEBUG|(__channel)|((__channel2)<<LOGGER_CHANNEL_BITS), \
				__FILE__, __LINE__, __FUNCTION__, 0, 0, __message, ##__VA_ARGS__)

#define log_trace(__channel, __message, ...)	\
	g_logger.write(LT_TRACE|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				0, 0, __message, ##__VA_ARGS__)

#define log_trace2(__channel, __channel2, __message, ...)	\
	g_logger.write(LT_TRACE|(__channel)|((__channel2)<<LOGGER_CHANNEL_BITS), \
				__FILE__, __LINE__, __FUNCTION__, 0, 0, __message, ##__VA_ARGS__)

/*
 * Log message and hex dump:
 * 		<message><dump>
 *
 * log_debug_bin(), log_debug_bin2(), log_trace_bin(), log_trace_bin2()
 * 		got one/two log channels and write string/hex data of the debug type
 */
#define log_debug_bin(__channel, __data, __length, __message, ...)	\
	g_logger.write(LT_DEBUG|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				__data, __length, __message, ##__VA_ARGS__)

#define log_debug_bin2(__channel, __channel2, __data, __length, __message, ...)	\
	g_logger.write(LT_DEBUG|(__channel)|((__channel2)<<LOGGER_CHANNEL_BIT_MAX), \
				__FILE__, __LINE__, __FUNCTION__, __data, __length, __message, ##__VA_ARGS__)

#define log_trace_bin(__channel, __data, __length, __message, ...)	\
	g_logger.write(LT_TRACE|(__channel), __FILE__, __LINE__, __FUNCTION__, \
				__data, __length, __message, ##__VA_ARGS__)

#define log_trace_bin2(__channel, __channel2, __data, __length, __message, ...)	\
	g_logger.write(LT_TRACE|(__channel)|((__channel2)<<LOGGER_CHANNEL_BIT_MAX), \
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
	g_logger.write(LT_DEBUG|L_ALWAYS_ENABLED, NULL, 0, NULL, NULL, 0, \
						__message, ##__VA_ARGS__)

#define log_dump_bin(__data, __length, __message, ...)	\
	g_logger.write(LT_DEBUG|L_ALWAYS_ENABLED, NULL, 0, NULL, \
						__data, __length, __message, ##__VA_ARGS__)


#define logger_enable(__typeChannel)		g_logger.enable(__typeChannel)
#define logger_disable(__typeChannel)		g_logger.disable(__typeChannel)
#define logger_is_enabled(__typeChannel)	g_logger.isEnabled(__typeChannel)

#endif /* __SHELL_LOGGER_COMPAT_H_INCLUDED__ */
