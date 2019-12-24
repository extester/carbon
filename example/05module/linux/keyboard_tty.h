/*
 *	Carbon Framework Examples
 *	Linux TTY Keyboard driver
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 26.01.2017 13:07:48
 *	    Initial revision.
 */

#ifndef __EXAMPLE_KEYBOARD_TTY_H_INCLUDED__
#define __EXAMPLE_KEYBOARD_TTY_H_INCLUDED__

#include <termios.h>

#include "../keyboard_driver.h"

class CKeyboardTty : public CKeyboardDriver
{
	private:
		struct termios		m_svTermios;	/* Saved terminal attributes */

    public:
		CKeyboardTty();
        virtual ~CKeyboardTty();

		virtual result_t init();
		virtual void terminate();

		virtual int getKey();
};

#endif /* __EXAMPLE_KEYBOARD_TTY_H_INCLUDED__ */
