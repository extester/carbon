/*
 *  Carbon/Contact module
 *  General Purpose Input/Output support base class
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 07.05.2018 15:51:01
 *      Initial revision.
 */

#ifndef __CONTACT_GPIO_H_INCLUDED__
#define __CONTACT_GPIO_H_INCLUDED__

#include "shell/types.h"
#include "shell/object.h"

class CGpio : public CObject
{
	public:
		enum {
			gpioIn = 0,
			gpioOut = 1
		};

    protected:
        int         	m_nPin;					/* Pin number */
        int				m_direction;			/* I/O direction, gpioIn/gpioOut */

    public:
        CGpio(int nPin, int direction);
        virtual ~CGpio();

    public:
		virtual int getPin() const { return m_nPin; }
		virtual int getDirection() const { return m_direction; }

        virtual int read(result_t* pnresult = 0) = 0;	/* Return: 0, 1, -1-on error */
        virtual result_t write(int nValue) = 0;
};

#endif /* __CONTACT_GPIO_H_INCLUDED__ */
