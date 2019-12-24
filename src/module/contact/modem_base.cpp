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
 *  Revision 1.0, 05.03.2013, 17:48:18
 *      Initial revision.
 *
 *  Revision 1.1, 08.05.2018 15:36:08
 *  	Ported to carbon/contact.
 */

#include "carbon/logger.h"

#include "contact/modem_base.h"


/*******************************************************************************
 * Class CModemBase
 */

/*
 * Format modem command dump line filtering the special characters
 *
 * 		pData		modem command
 * 		size		modem command length, bytes
 * 		strPref		dump line prefix (optional)
 */
void CModemBase::dumpModemLine(const void* pData, size_t nSize, const char* strPref) const
{
	char			buf[128] = "\0";
	int				len = 0, lmax = sizeof(buf)-1;
	size_t			rsize = nSize;
	const uint8_t*	p = (const uint8_t*)pData;

	if ( pData == 0 )  {
		return;
	}

	len = _tsnprintf(buf, lmax, "%s:(%zu) ", strPref, nSize);

	if ( nSize == 0 )  {
		_tsnprintf(&buf[len], lmax-len, "%s\n", "<empty>");
		return;
	}

	while ( len < lmax && rsize > 0 )  {
		if ( *p == '\r' || *p == '\n' )  {
			_tsnprintf(&buf[len], lmax-len, "%s", *p == '\r' ? "<cr>" : "<ln>");
			len = (int)_tstrlen(buf);
		} else
		if ( *p >= 0x20 && *p < 0x7f )  {
			_tsnprintf(&buf[len], lmax-len, "%c", *p);
			len = (int)_tstrlen(buf);
		} else {
			_tsnprintf(&buf[len], lmax-len, "'0x%02X'", (*p)&0xff);
			len = (int)_tstrlen(buf);
		}
		p++; rsize--;
	}

	buf[len] = '\n';
	buf[len+1] = '\0';

	log_dump(buf);
}
