/*
 *	Carbon Framework Examples
 *	Keyboard driver base class
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 26.01.2017 13:02:46
 *	    Initial revision.
 */

#ifndef __EXAMPLE_KEYBOARD_DRIVER_H_INCLUDED__
#define __EXAMPLE_KEYBOARD_DRIVER_H_INCLUDED__

#include "carbon/carbon.h"

/*
 * Keyboard driver virtual base class
 */
class CKeyboardDriver
{
	public:
		CKeyboardDriver() {}
		virtual ~CKeyboardDriver() {}

	public:
		virtual result_t init() = 0;
		virtual void terminate() = 0;

        virtual int getKey() = 0;
};

/*
 * Keyboard driver factory
 *
 * To Define: doCreateDriver() in the user application
 */
class CKeyboardDriverFactory
{
	public:
		static CKeyboardDriver* createDriver()
		{
			CKeyboardDriver*	pDriver;
			result_t			nresult;

			pDriver = doCreateDriver();
			if ( pDriver )  {
				nresult = pDriver->init();
				if ( nresult != ESUCCESS )  {
					delete pDriver;
					pDriver = 0;
				}
			}

			return pDriver;
		}

		static void deleteDriver(CKeyboardDriver* pDriver)
		{
			if ( pDriver ) {
				pDriver->terminate();
				delete pDriver;
			}
		}

	private:
		static CKeyboardDriver* doCreateDriver();
};

#endif /* __EXAMPLE_KEYBOARD_DRIVER_H_INCLUDED__ */
