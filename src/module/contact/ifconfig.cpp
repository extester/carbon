/*
 *  Carbon/Contact module
 *  Generic network interface configuration
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.08.2017 18:03:04
 *      Initial revision.
 */

#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>

#include "shell/shell.h"
#include "contact/ifconfig.h"

/*******************************************************************************
 * CIfConfig class
 */

result_t CIfConfig::openSocket(int& hSocket) const
{
	int 		hSock;
	result_t	nresult = ESUCCESS;

	hSock = ::socket(AF_INET, SOCK_DGRAM, 0);
	if ( hSock >= 0 )  {
		hSocket = hSock;
	}
	else {
		nresult = errno;
		log_debug(L_GEN, "[ifconfig(%s)] failed to create temporary socket, result %d\n",
				  	m_strIface.c_str(), nresult);
		hSocket = 0; /* avoid warning 'may be uninitialised' */
	}

	return nresult;
}

void CIfConfig::closeSocket(int hSocket) const
{
	::close(hSocket);
}

result_t CIfConfig::ioctl(int hSocket, unsigned long type, struct ifreq* ifr)
{
	int 		intVal;
	result_t	nresult = ESUCCESS;

	intVal = ::ioctl(hSocket, type, ifr);
	if ( intVal < 0 )  {
		nresult = errno;
		log_debug(L_GEN, "[ifconfig(%s)] ioctl() failed on temporary socket, type %d, result %d\n",
				  	m_strIface.c_str(), type, nresult);
	}

	return nresult;
}

result_t CIfConfig::ioctl_const(int hSocket, unsigned long type, struct ifreq* ifr) const
{
	int 		intVal;
	result_t	nresult = ESUCCESS;

	intVal = ::ioctl(hSocket, type, ifr);
	if ( intVal < 0 )  {
		nresult = errno;
		log_debug(L_GEN, "[ifconfig(%s)] ioctl() failed on temporary socket, type %d, result %d\n",
				  m_strIface.c_str(), type, nresult);
	}

	return nresult;
}

boolean_t CIfConfig::isPresnt() const
{
	struct ifreq	ifr;
	int				hSocket;
	result_t		nresult;
	boolean_t		bPresent = FALSE;

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS )  {
		copyString(ifr.ifr_name, m_strIface.c_str(), IFNAMSIZ);
		nresult = ioctl_const(hSocket, SIOCGIFFLAGS, &ifr);
		if ( nresult == ESUCCESS )  {
			bPresent = TRUE;
		}

		closeSocket(hSocket);
	}

	return bPresent;
}

/*
 * Check if an interface is UP/DOWN
 *
 * 		bUp			TRUE: interface is UP, FALSE: interface is DOWN [out]
 *
 * Return: ESUCCESS, ...
 */
result_t CIfConfig::isUp(boolean_t& bUp) const
{
	int				hSocket;
	struct ifreq	ifr;
	result_t		nresult;

	bUp = FALSE;

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS )  {
		copyString(ifr.ifr_name, m_strIface.c_str(), IFNAMSIZ);
		nresult = ioctl_const(hSocket, SIOCGIFFLAGS, &ifr);
		if ( nresult == ESUCCESS )  {
			bUp = (ifr.ifr_flags & IFF_UP) != 0;
		}

		closeSocket(hSocket);
	}

	return nresult;
}

/*
 * Bring interface UP/DOWN
 *
 * 		bUp			TRUE: bring interface UP, FALSE: bring interface DOWN
 *
 * Return: ESUCCESS, ...
 */
result_t CIfConfig::up(boolean_t bUp)
{
	int				hSocket;
	struct ifreq	ifr;
	result_t		nresult;

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS )  {
		copyString(ifr.ifr_name, m_strIface.c_str(), IFNAMSIZ);
		nresult = ioctl(hSocket, SIOCGIFFLAGS, &ifr);
		if ( nresult == ESUCCESS )  {
			boolean_t	isUp = (ifr.ifr_flags & IFF_UP) != 0;

			if ( isUp != bUp )  {
				if ( bUp )  {
					ifr.ifr_flags |= IFF_UP;
				}
				else {
					ifr.ifr_flags &= ~IFF_UP;
				}

				nresult = ioctl(hSocket, SIOCSIFFLAGS, &ifr);
				if ( nresult != ESUCCESS )  {
					log_debug(L_GEN, "[ifconfig(%s)] interface UP failed, result %d\n",
							  	m_strIface.c_str(), nresult);
				}
			}
		}
		else {
			log_debug(L_GEN, "[ifconfig(%s)] ioctl UP failed, result %d\n",
					  			m_strIface.c_str(), nresult);
		}

		closeSocket(hSocket);
	}

	return nresult;
}

/*
 * Setup interface IP address and MASK
 *
 *		ipAddr		IP address
 *		ipMask		net mask
 *
 * Return: ESUCCESS, ...
 */
result_t CIfConfig::config(const CNetHost& ipAddr, const CNetHost& ipMask)
{
	int					hSocket;
	struct sockaddr_in	sin;
	struct ifreq		ifr;
	result_t			nresult;

	if ( !ipAddr.isValid() ) {
		log_debug(L_GEN, "[ifconfig(%s)] invalid IP address specified\n", m_strIface.c_str());
		return EINVAL;
	}

	if ( !ipMask.isValid() )  {
		log_debug(L_GEN, "[ifconfig(%s)] invalid MASK specified\n", m_strIface.c_str());
		return EINVAL;
	}

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS )  {
		_tbzero_object(sin);
		sin.sin_family = AF_INET;
		sin.sin_addr = ipAddr;

		copyString(ifr.ifr_name, m_strIface.c_str(), IFNAMSIZ);
		_tmemcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));

		nresult = ioctl(hSocket, SIOCSIFADDR, &ifr);
		if ( nresult == ESUCCESS )  {
			sin.sin_family = AF_INET;
			sin.sin_addr = ipMask;
			_tmemcpy(&ifr.ifr_netmask, &sin, sizeof(struct sockaddr));

			nresult = ioctl(hSocket, SIOCSIFNETMASK, &ifr);
			if ( nresult != ESUCCESS )  {
				log_debug(L_GEN, "[ifconfig(%s)] set NETMASK failed, result %d\n",
						  m_strIface.c_str(), nresult);
			}
		}
		else {
			log_debug(L_GEN, "[ifconfig(%s)] set IP address failed, result %d\n",
					  m_strIface.c_str(), nresult);
		}

		closeSocket(hSocket);
	}

	return nresult;
}

/*
 * Setup interface IP address and MASK
 *
 *		strIpAddr		IP address
 *		strIpMask		net mask
 *
 * Return: ESUCCESS, ...
 */
result_t CIfConfig::config(const char* strIpAddr, const char* strIpMask)
{
	CNetHost	ipAddr, ipMask;

	if ( strIpAddr && strIpAddr[0] != '\0' )  {
		ipAddr = CNetHost(strIpAddr);
	}
	if ( strIpMask && strIpMask[0] != '\0' )  {
		ipMask = CNetHost(strIpMask);
	}

	return config(ipAddr, ipMask);
}

/*
 * Setup MAC address
 *
 * 		pMacData			ETHER_ADDR_LEN-bytes length binary data pointer
 *
 * Return: ESUCCESS, ...
 */
result_t CIfConfig::setMacAddr(const uint8_t* pMacData)
{
	int				hSocket;
	struct ifreq	ifr;
	int 			i;
	result_t		nresult;

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS ) {
		_tbzero_object(ifr);
		copyString(ifr.ifr_name, m_strIface.c_str(), IFNAMSIZ);
		for (i=0; i<ETHER_ADDR_LEN; i++) ifr.ifr_hwaddr.sa_data[i] = pMacData[i];
		ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

		nresult = ioctl(hSocket, SIOCSIFHWADDR, &ifr);
		if ( nresult != ESUCCESS )  {
			log_debug(L_GEN, "[ifconfig(%s)] set MAC address failed, result %d\n",
					  	m_strIface.c_str(), nresult);
		}

		closeSocket(hSocket);
	}

	return nresult;
}

/*
 * Setup MAC address
 *
 * 		strMac		mac address: "nn.nn.nn.nn.nn.nn", where nn in hex
 *
 * Return: ESUCCESS, EINVAL, ...
 */
result_t CIfConfig::setMacAddr(const char* strMac)
{
	uint8_t			mac[ETHER_ADDR_LEN];
	uint32_t		xx;
	int 			i;
	const char*		s;
	char*			endp;
	result_t		nresult = EINVAL;

	if ( !strMac )  {
		return EINVAL;		/* No string specified */
	}

	i = 0;
	s = strMac;

	while ( i < ETHER_ADDR_LEN )  {
		SKIP_CHARS(s, " \t");
		xx = _tstrtoul(s, &endp, 16);
		if ( *endp == ':' || *endp == ' ' || *endp == '\t' ||
			(*endp == '\0' && i == (ETHER_ADDR_LEN-1)) )
		{
			if ( xx > 255 )  {
				/* Number overflow */
				break;
			}

			mac[i] = xx&0xff;
			i++;
		}
		else {
			/* Invalid hexadecimal number */
			break;
		}

		s = endp+1;
	}

	if ( i == ETHER_ADDR_LEN )  {
		nresult = setMacAddr(mac);
	}

	return nresult;
}

/*
 * Get MAC address
 *
 * 		pMacAddr		ETHER_ADDR_LEN-bytes length binary data pointer
 *
 * Return: ESUCCESS, ...
 */
result_t CIfConfig::getMacAddr(uint8_t* pMacAddr) const
{
	int				hSocket;
	struct ifreq	ifr;
	result_t		nresult;

	shell_assert(pMacAddr);

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS ) {
		_tbzero_object(ifr);
		copyString(ifr.ifr_name, m_strIface.c_str(), IFNAMSIZ);
		ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;

		nresult = ioctl_const(hSocket, SIOCGIFHWADDR, &ifr);
		if ( nresult == ESUCCESS )  {
			_tmemcpy(pMacAddr, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
		}

		closeSocket(hSocket);
	}

	return nresult;
}

/*
 * Get MAC address
 *
 * 		strMac		MAC address [out], format: "nn:nn:nn:nn:nn:nn"
 *
 * Return: ESUCCESS, ...
 */
result_t CIfConfig::getMacAddr(CString& strMac) const
{
	uint8_t		mac[ETHER_ADDR_LEN];
	result_t	nresult;

	nresult = getMacAddr(mac);
	if ( nresult == ESUCCESS )  {
		strMac.format("%02X:%02X:%02X:%02X:%02X:%02X",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	return nresult;
}

/*
 * Get interface IP address
 *
 * 		ipAddr		IP address [out]
 *
 * Return: ESUCCESS, ...
 */
result_t CIfConfig::getIp(CNetHost& ipAddr) const
{
	int					hSocket;
	struct ifreq		ifr;
	struct sockaddr_in	sin;
	result_t			nresult;

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS ) {
		copyString(ifr.ifr_name, m_strIface.c_str(), IFNAMSIZ);

		nresult = ioctl_const(hSocket, SIOCGIFADDR, &ifr);
		if ( nresult == ESUCCESS) {
			_tmemcpy(&sin, &ifr.ifr_addr, sizeof(struct sockaddr));
			ipAddr = sin.sin_addr;
		}
	}

	return nresult;
}

/*
 * Get interface network MASK
 *
 * 		ipMask		network MASK [out]
 *
 * Return: ESUCCESS, ...
 */
result_t CIfConfig::getMask(CNetHost& ipMask) const
{
	int					hSocket;
	struct ifreq		ifr;
	struct sockaddr_in	sin;
	result_t			nresult;

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS ) {
		copyString(ifr.ifr_name, m_strIface.c_str(), IFNAMSIZ);

		nresult = ioctl_const(hSocket, SIOCGIFNETMASK, &ifr);
		if ( nresult == ESUCCESS) {
			_tmemcpy(&sin, &ifr.ifr_addr, sizeof(struct sockaddr));
			ipMask = sin.sin_addr;
		}
	}

	return nresult;
}
