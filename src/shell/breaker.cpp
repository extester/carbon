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
 *  Revision 1.0, 21.07.2016 14:46:16
 *      Initial revision.
 */

#include <fcntl.h>
#include <unistd.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/breaker.h"

/*******************************************************************************
 * CFileBreaker class
 */

CFileBreaker::CFileBreaker()
{
	m_fds[0] = m_fds[1] = -1;
}

CFileBreaker::~CFileBreaker()
{
	disable();
}

/*
 * Enable file breaker
 *
 * Return: ESUCCES, ...
 */
result_t CFileBreaker::enable()
{
	result_t	nresult = ESUCCESS;

	if ( !isEnabled() )  {
		int		retVal;

		retVal = ::pipe2(m_fds, O_NONBLOCK);
		if ( retVal < 0 )  {
			nresult = errno;
			log_error(L_GEN, "[file_breaker] failed to create pipe2(), result %d\n",
					  	nresult);
		}
	}

	return nresult;
}

/*
 * Disable file breaker
 *
 * Return: ESUCCESS, ...
 */
result_t CFileBreaker::disable()
{
	result_t	nresult = ESUCCESS;

	if ( isEnabled() )  {
		int 	retVal;

		retVal = ::close(m_fds[0]);
		if ( retVal < 0 ) {
			nresult = errno;
		}

		retVal = ::close(m_fds[1]);
		if ( retVal < 0 )  {
			nresult_join(nresult, errno);
		}

		if ( nresult != ESUCCESS )  {
			log_error(L_GEN, "[file_breaker] failed to close fd(s), result %d\n",
					  nresult);
		}

		m_fds[0] = m_fds[1] = -1;
	}

	return nresult;
}

/*
 * Clear last BREAK signal on the file descriptor
 */
void CFileBreaker::reset()
{
	shell_assert(isEnabled());
	if ( isEnabled() ) {
		char 	b[32];

		while ( ::read(m_fds[0], b, sizeof(b)) > 0 ) ;
	}
}

/*
 * Send BREAK signal to the awaiting I/O activity descriptor
 */
void CFileBreaker::_break()
{
	if ( isEnabled() ) {
		char 	b[1] = { 0 };
		ssize_t n;

		n = ::write(m_fds[1], b, 1);
		shell_unused(n);
	}
}
