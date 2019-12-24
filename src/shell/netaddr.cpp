/*
 *  Shell library
 *  Network (IP V4) destination address and port
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 20.04.2015 23:22:43
 *      Initial revision.
 *
 *  Revision 1.1, 05.29.2015 15:48:02
 *  	Added CNetHost class
 */

#include <arpa/inet.h>

#include "shell/logger.h"
#include "shell/net/netutils.h"
#include "shell/netaddr.h"


/*******************************************************************************
 * CNetHost
 */

/*
 * Setup host from string
 *
 * 		strHost			host ip ("nnn.nnn.nnn.nnn" or "eth0")
 *
 * Return:
 * 		TRUE		host IP is valid
 * 		FALSE		host IP is invalid
 */
boolean_t CNetHost::setHost(const char* strHost)
{
	struct in_addr	inaddr;
	int 			retVal;
	const char*		p;
	result_t		nresult;

	p = _tstrchr(strHost, '.');
	if ( !p )  {
		/* Try to find interface name */
		nresult = getInterfaceIp(strHost, &inaddr);
		if ( nresult == ESUCCESS )  {
			m_ip = inaddr.s_addr;
			m_bValid = TRUE;
			return TRUE;
		}
		/* fall down */
	}

	/* Try to parse ip address */
	retVal = inet_pton(AF_INET, strHost, &inaddr);
	if ( retVal > 0 ) {
		/* Success */
		m_ip = inaddr.s_addr;
		m_bValid = TRUE;
	}
	else {
		/*if( strHost != 0 && *strHost != '\0' ) {
			log_debug(L_GEN, "[net_host] invalid ip address: '%s'\n", strHost);
		}*/
		m_ip = 0;
		m_bValid = FALSE;
	}

	return m_bValid;
}

void CNetHost::strHost_r(char* strHost, size_t length) const
{
	struct in_addr	inaddr;
	const char*		p;
	socklen_t 		len;

	shell_assert(length > 0);

	if ( length > 0 ) {
		*strHost = '\0';

		inaddr.s_addr = m_ip;
		len = (socklen_t) length - 1;
		p = inet_ntop(AF_INET, &inaddr, strHost, len);
		if ( p == NULL ) {
			log_debug(L_GEN, "[net_host] failed to inet_ntop(), ip=0x%X\n", m_ip);
			*strHost = '\0';
		}
	}
}


/*******************************************************************************
 * CNetAddr
 */

void CNetAddr::strAddr_r(char* strAddr, size_t length) const
{
	strHost_r(strAddr, length);
	if ( length > 0 ) {
		size_t	len;

		len = _tstrlen(m_strBuf);
		if ( (len + 2) < length ) {
			_tsnprintf(&strAddr[len], length - 1 - len, ":%d", ntohs(m_port));
		}
	}
}
