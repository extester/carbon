/*
 *  Shell library
 *  I/O operations breaker
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 21.07.2016 14:39:37
 *      Initial revision.
 */

#ifndef __SHELL_BREAKER_H_INCLUDED__
#define __SHELL_BREAKER_H_INCLUDED__

#include "shell/types.h"

class CFileBreaker
{
	protected:
		int 	m_fds[2];

	public:
		CFileBreaker();
		virtual ~CFileBreaker();

	public:
		result_t enable();
		result_t disable();
		boolean_t isEnabled() const { return m_fds[0] >= 0; }

		virtual void reset();
		virtual void _break();

		virtual int getRHandle() { return m_fds[0]; }
		virtual int getWHandle() { return m_fds[1]; }
};

#endif /* __SHELL_BREAKER_H_INCLUDED__ */
