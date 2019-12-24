/*
 *  Shell library
 *  Logger (embedded version)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.03.2017 11:19:19
 *      Initial revision.
 */

#include <stdio.h>
#include <stdlib.h>

#include "shell/config.h"
#include "shell/shell.h"
#include "shell/logger.h"

#include "shell/embed/console_output.h"

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
	va_list     	args;

	//char	buf[CARBON_LOGGER_BUFFER_LENGTH/2];

	va_start(args, strMessage);
	if ( strFilename ) {
		console_printf("%s:%d: ", strFilename, line);
		console_vprintf(strMessage, args);
		//snprintf(buf, sizeof(buf), "%s:%d: %s", strFilename, line, strMessage);
	}
	else {
		console_vprintf(strMessage, args);
		//snprintf(buf, sizeof(buf), "%s", strMessage);
	}
	//console_vprintf(buf, args);
	va_end(args);
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
	/*if ( g_pLogger )  {
		g_pLogger->enable(type_channel);
	}*/
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
	/*if ( g_pLogger )  {
		g_pLogger->disable(type_channel);
	}*/
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
	return TRUE; //g_pLogger ? g_pLogger->isEnabled(type_channel) : FALSE;
}

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
	/*if ( g_pLogger == 0 )  {
		appender_handle_t	hAppender;

		g_pLogger = new CLogger();
		if ( bDefaultLogger )  {
			hAppender = logger_addStdoutAppender();
			if ( hAppender == LOGGER_APPENDER_NULL )  {
				logger_syslog_impl("FAILED TO CREATE DEFAULT APPENDER, LOGGER MAY NOT BE AVAILABLE!\n");
			}
		}
	}*/
}

/*
 * Free logger resources and delete all appenders
 */
void logger_terminate()
{
	/*if ( g_pLogger )  {
		SAFE_DELETE(g_pLogger);
	}*/
}
