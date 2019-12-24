/*
 *  Carbon/Contact library
 *  GSM modem base class
 *
 *  Copyright (c) 2013-2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.03.2013, 17:45:51
 *      Initial revision.
 *
 *  Revision 1.1, 08.05.2018 15:36:08
 *  	Ported to carbon/contact.
 */

#ifndef __CONTACT_MODEM_BASE_H_INCLUDED__
#define __CONTACT_MODEM_BASE_H_INCLUDED__

#include "shell/shell.h"
#include "shell/hr_time.h"
#include "shell/object.h"


class CModemBase : public CObject
{
	protected:
		CModemBase(const char* strName) : CObject(strName) {}

	public:
		virtual ~CModemBase() {}

	public:
		/*
		 * Debug functions
		 */
		virtual void dumpModemLine(const void* pData, size_t nSize,
							   const char* strPref = "") const;

	protected:
		/*
		 * Hardware related functions
		 */
		virtual result_t devicePowerOff() = 0;
		virtual result_t devicePowerOn() = 0;

		virtual result_t deviceSend(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout) = 0;
		virtual result_t deviceRecv(unsigned char* pByte, hr_time_t hrTimeout) = 0;

};

#endif /* __CONTACT_MODEM_BASE_H_INCLUDED__ */
