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
 *  Revision 1.0, 07.05.2018 16:15:21
 *      Initial revision.
 */

#include "shell/file.h"
#include "shell/logger.h"

#include "contact/gpio_linux.h"

/*******************************************************************************
 * CGpioLinux class
 */

result_t CGpioLinux::exportGpio()
{
	char		strTmp[256];
	result_t	nresult;

	_tsnprintf(strTmp, sizeof(strTmp), "%u\n", m_nPin);
	nresult = CFile::writeFile("/sys/class/gpio/export", strTmp, strlen(strTmp));
	if ( nresult == ESUCCESS ) {
		nresult = setDirection();
	}

	return nresult;
}

result_t CGpioLinux::setDirection()
{
	char		strTmp[256];
	result_t	nresult;

	_tsnprintf(strTmp, sizeof(strTmp), "/sys/class/gpio/gpio%u/direction", m_nPin);
	if ( m_direction == CGpio::gpioIn )  {
		nresult = CFile::writeFile(strTmp, "in", 2);
	}
	else {
		nresult = CFile::writeFile(strTmp, "out", 3);
	}

	m_bDirection = TRUE;
	return nresult;
}

int CGpioLinux::read(result_t* pnresult)
{
	int 		nValue = -1;
	result_t	nresult = EPERM;

	if ( getDirection() == CGpio::gpioIn ) {
		char		strValue[256];
		char        buf[32];
		char*   	pEnd;
		int     	value;

		_tsnprintf(strValue, sizeof(strValue), "/sys/class/gpio/gpio%u/value", m_nPin);

		if ( !CFile::fileExists(strValue, R_OK) )  {
			exportGpio();
		}
		else {
			if ( !m_bDirection )  {
				setDirection();
			}
		}

		_tbzero_object(buf);
		nresult = CFile::readFile(strValue, buf, sizeof(buf)-1);
		if ( nresult == ESUCCESS )  {
			value = (int)strtol(buf, &pEnd, 10);
			if ( pEnd != buf )  {
				nValue = value;
				nresult = ESUCCESS;
			}
			else  {
				nresult = EINVAL;
			}
		}
	}

	if ( pnresult )  {
		*pnresult = nresult;
	}

	return nValue;
}

result_t CGpioLinux::write(int nValue)
{
	result_t	nresult = EPERM;

	if ( getDirection() == CGpio::gpioOut ) {
		char		strValue[256];
		char        buf[32];
		int			n;

		_tsnprintf(strValue, sizeof(strValue), "/sys/class/gpio/gpio%u/value", m_nPin);

		if ( !CFile::fileExists(strValue, W_OK) )  {
			exportGpio();
		}
		else {
			if ( !m_bDirection )  {
				setDirection();
			}
		}

		n = _tsnprintf(buf, sizeof(buf), "%u\n", nValue);
		nresult = CFile::writeFile(strValue, buf, (size_t)n);
	}

	return nresult;
}
