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
 *  Revision 1.0, 07.05.2018 15:54:01
 *      Initial revision.
 */

#include "shell/shell.h"

#include "contact/gpio.h"

/*******************************************************************************
 * CGpio base class
 */

CGpio::CGpio(int nPin, int direction) :
	CObject(),
	m_nPin(nPin),
	m_direction(direction)
{
	char 	strTmp[256];

	_tsnprintf(strTmp, sizeof(strTmp), "gpio-%u", nPin);
	setName(strTmp);
}

CGpio::~CGpio()
{
}
