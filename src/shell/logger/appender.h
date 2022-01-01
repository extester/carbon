/*
 *  Shell library
 *  Logger Appender base class
 *
 *  Copyright (c) 2021 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 21.01.2021 16:35:29
 *      Initial revision.
 *
 */

#ifndef __SHELL_LOGGER_APPENDER_H_INCLUDED__
#define __SHELL_LOGGER_APPENDER_H_INCLUDED__

#include "shell/types.h"

class CAppender
{
	protected:
		CAppender() {}

	public:
		virtual ~CAppender() noexcept = default;

	public:
		virtual result_t init() = 0;
		virtual void terminate() = 0;
		virtual result_t append(const void* pData, size_t nLength) = 0;
};

#endif /* __SHELL_LOGGER_APPENDER_H_INCLUDED__ */
