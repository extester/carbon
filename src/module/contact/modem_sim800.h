/*
 *  Carbon/Contact library
 *  SIM800 GSM modem
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.05.2018, 15:15:37
 *      Initial revision.
 */

#ifndef __CONTACT_MODEM_SIM800_H_INCLUDED__
#define __CONTACT_MODEM_SIM800_H_INCLUDED__

#include "contact/modem_sim900r.h"

class CModemSim800 : public CModemSim900r
{
	public:
		CModemSim800(const char* strName);
		virtual ~CModemSim800();

	protected:
		virtual boolean_t doUnsolicited(const char* strResponse, size_t size);
};

#endif /* __CONTACT_MODEM_SIM800_H_INCLUDED__ */
