/*
 *	Carbon Framework
 *	Linux TTY Keyboard driver
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 24.01.2017 11:19:47
 *		Initial revision.
 */

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "keyboard_tty.h"

/*
 * Keyboard driver constructor
 */
CKeyboardTty::CKeyboardTty() :
	CKeyboardDriver()
{
}

/*
 * Keyboard driver destructor
 */
CKeyboardTty::~CKeyboardTty()
{
}

/*
 * Keyboard driver API function: wait and return key code
 *
 * Return:
 * 		-1		EOF detected
 * 		n		pressed key code
 */
int CKeyboardTty::getKey()
{
	int 	c;

	c = getchar();
	return c != EOF ? c : -1;
}

/*
 * Driver initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CKeyboardTty::init()
{
	struct termios  newtio;
	int				retVal;
	result_t		nresult;

	retVal = tcgetattr(STDIN_FILENO, &m_svTermios);
	if ( retVal < 0 )  {
		nresult = errno;
		log_error(L_GEN, "[key_tty] tcgetattr() failed, result: %d\n", nresult);
		return nresult;
	}

	newtio = m_svTermios;
	cfmakeraw(&newtio);

	retVal = tcsetattr(STDIN_FILENO, TCSANOW, &newtio);
	if ( retVal < 0 )  {
		nresult = errno;
		log_error(L_GEN, "[key_tty] tcsetattr() failed, result: %d\n", nresult);
		return nresult;
	}

	return ESUCCESS;
}

/*
 * Driver termination
 */
void CKeyboardTty::terminate()
{
	tcsetattr(STDIN_FILENO, TCSANOW, &m_svTermios);
}

/*******************************************************************************/

/*
 * Required by CKeyboardDriverFactory
 *
 * Return: driver object pointer or 0
 */
CKeyboardDriver* CKeyboardDriverFactory::doCreateDriver()
{
	return new CKeyboardTty;
}
