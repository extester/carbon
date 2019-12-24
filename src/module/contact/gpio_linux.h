/*
 *  Carbon/Contact module
 *  Linux General Purpose Input/Output class
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 07.05.2018 16:10:11
 *      Initial revision.
 */

#ifndef __CONTACT_GPIO_LINUX_H_INCLUDED__
#define __CONTACT_GPIO_LINUX_H_INCLUDED__

#include "contact/gpio.h"

class CGpioLinux : public CGpio
{
	protected:
		boolean_t		m_bDirection;

    public:
        CGpioLinux(int nPin, int direction) :
			CGpio(nPin, direction),
			m_bDirection(FALSE)
		{
		}

        virtual ~CGpioLinux()
		{
		}

    public:
		virtual int read(result_t* pnresult = 0);
		virtual result_t write(int nValue);

	protected:
		result_t exportGpio();
		result_t setDirection();
};

#endif /* __CONTACT_GPIO_LINUX_H_INCLUDED__ */
