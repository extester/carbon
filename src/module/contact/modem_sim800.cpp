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
 *  Revision 1.0, 11.05.2018, 15:18:41
 *      Initial revision.
 */
/*
 * Sim900r and Sim800 differences:
 *
 *		1) Unsolicited 'SMS Ready' on 57600 speed (instead of 'CPIN Ready')
 */

#include "carbon/logger.h"

#include "contact/modem_sim800.h"


/*******************************************************************************
 * Class CModemSim800
 */

CModemSim800::CModemSim800(const char* strName) :
	CModemSim900r(strName)
{
}

CModemSim800::~CModemSim800()
{
}

boolean_t CModemSim800::doUnsolicited(const char* strResponse, size_t size)
{
	boolean_t	bProcessed;

	bProcessed = CModemSim900r::doUnsolicited(strResponse, size);
	if ( !bProcessed )  {
		if ( size > 12 ) {
			if ( _tmemcmp(strResponse, "\r\nSMS Ready\r\n", 13) == 0 ) {
				if ( m_nCardState == CARD_STATE_UNKNOWN )  {
					m_nCardState = CARD_STATE_PRESENT;
					onCpinReady();
				}
			}
			bProcessed = TRUE;
		}

		if ( logger_is_enabled(LT_DEBUG|L_MODEM_IO) && bProcessed ) {
			char	tmp[128];
			_tsnprintf(tmp, sizeof(tmp), "[modem(%s)] URC", getName());
			dumpModemLine(strResponse, size, tmp);
		}
	}

	return bProcessed;
}
