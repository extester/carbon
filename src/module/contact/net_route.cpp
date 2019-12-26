/*
 *  Carbon/Contact module
 *  Network routing
 *
 *  Copyright (c) 2017-2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 26.09.2017 14:53:00
 *      Initial revision.
 *
 *  Revision 1.1, 16.05.2018 17:55:25
 *  	Added addRoute:2, delRoute:2, dump().
 */

#include <unistd.h>
#include <net/route.h>
#include <sys/ioctl.h>

#include "shell/file.h"
#include "shell/net/netutils.h"

#include "carbon/cstring.h"
#include "contact/net_route.h"

/*******************************************************************************
 * CNetRoute class
 */

CNetRoute::CNetRoute() :
	CModule("net-route")
{
}

CNetRoute::~CNetRoute()
{
}

result_t CNetRoute::openSocket(int& hSocket) const
{
	int 		hSock;
	result_t	nresult = ESUCCESS;

	hSock = ::socket(AF_INET, SOCK_DGRAM, 0);
	if ( hSock >= 0 )  {
		hSocket = hSock;
	}
	else {
		nresult = errno;
		log_debug(L_NET_ROUTE, "[netroute] failed to create temporary socket, result %d\n",
				  nresult);
		hSocket = 0; /* avoid warning 'may be uninitialised' */
	}

	return nresult;
}

void CNetRoute::closeSocket(int hSocket) const
{
	::close(hSocket);
}

result_t CNetRoute::doRoute(int hSocket, boolean_t bAdd, const char* strIface,
				const CNetHost& destAddr, const CNetHost& gwAddr, const CNetHost& destMask)
{
	struct rtentry		route;
	struct sockaddr_in*	addr;
	int 				retVal;
	result_t			nresult = ESUCCESS;

	shell_assert(hSocket);

	_tbzero_object(route);

	addr = (struct sockaddr_in*)&route.rt_dst;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = destAddr;
	addr->sin_port = 0;

	addr = (struct sockaddr_in*)&route.rt_gateway;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = gwAddr;
	addr->sin_port = 0;

	addr = (struct sockaddr_in*)&route.rt_genmask;
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = destMask;

	route.rt_dev = const_cast<char*>(strIface);
	route.rt_flags = RTF_UP | RTF_GATEWAY;
	route.rt_metric = 0;

	retVal = ::ioctl(hSocket, bAdd ? SIOCADDRT : SIOCDELRT, &route);
	if ( retVal < 0 )  {
		nresult = errno;
		log_error(L_NET_ROUTE, "[netroute] failed to %s route, iface: %s, gw %s, result %d\n",
				  	bAdd ? "add" : "delete", strIface ? strIface : "<none>",
				  	gwAddr.c_str(), nresult);
	}

	return nresult;
}

result_t CNetRoute::delDefaultGateway(int hSocket)
{
	result_t	nresult;

	nresult = doRoute(hSocket, FALSE, NULL, "0.0.0.0", "0.0.0.0", "0.0.0.0");
	return nresult;
}

result_t CNetRoute::addRoute(const char* strIface, const CNetHost& destAddr,
				  const CNetHost& gwAddr, const CNetHost& destMask)
{
	int 		hSocket;
	result_t	nresult;

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS )  {
		log_trace(L_NET_ROUTE, "[netroute] adding route: iface: %s, dest: %s, mask: %s, gw: %s\n",
				  	strIface, destAddr.c_str(), destMask.c_str(), gwAddr.c_str());

		nresult = doRoute(hSocket, TRUE, strIface, destAddr, gwAddr, destMask);
		closeSocket(hSocket);
	}

	return nresult;
}

result_t CNetRoute::addRoute(const char* strIface, const CNetHost& destAddr)
{
	struct in_addr		inaddr;
	result_t			nresult;

	if ( !destAddr.isValid() ) {
		log_error(L_NET_ROUTE, "[netroute] add: invalid destination\n");
		return EINVAL;
	}

	nresult = getInterfaceIp(strIface, &inaddr);
	if ( nresult != ESUCCESS )  {
		log_error(L_NET_ROUTE, "[netroute] add: invalid iface '%s'\n", strIface);
		return nresult;
	}

	nresult = addRoute(strIface, destAddr, CNetHost(inaddr), CNetHost("255.255.255.255"));
	return nresult;
}

result_t CNetRoute::delRoute(const char* strIface, const CNetHost& destAddr,
				  const CNetHost& gwAddr, const CNetHost& destMask)
{
	int 		hSocket;
	result_t	nresult;

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS )  {
		log_trace(L_NET_ROUTE, "[netroute] delete route: iface: %s, dest: %s, mask: %s, gw: %s\n",
				  strIface, destAddr.c_str(), destMask.c_str(), gwAddr.c_str());

		nresult = doRoute(hSocket, FALSE, strIface, destAddr, gwAddr, destMask);
		closeSocket(hSocket);
	}

	return nresult;
}

result_t CNetRoute::delRoute(const char* strIface, const CNetHost& destAddr)
{
	struct in_addr		inaddr;
	result_t			nresult;

	if ( !destAddr.isValid() ) {
		log_error(L_NET_ROUTE, "[netroute] del: invalid destination\n");
		return EINVAL;
	}

	nresult = getInterfaceIp(strIface, &inaddr);
	if ( nresult != ESUCCESS )  {
		log_error(L_NET_ROUTE, "[netroute] del: invalid iface '%s'\n", strIface);
		return nresult;
	}

	nresult = delRoute(strIface, destAddr, CNetHost(inaddr), CNetHost("255.255.255.255"));
	return nresult;
}

result_t CNetRoute::setDefaultGateway(const char* strIface, const CNetHost& gwAddr)
{
	CNetHost	zeroAddr("0.0.0.0");
	int 		hSocket;
	result_t	nresult;

	nresult = openSocket(hSocket);
	if ( nresult == ESUCCESS )  {
		log_trace(L_NET_ROUTE, "[netroute] adding default route for iface %s: %s\n",
				  	strIface, gwAddr.c_str());

		nresult = delDefaultGateway(hSocket);
		if ( nresult != ESUCCESS )  {
			log_warning(L_NET_ROUTE, "[netroute] failed to delete default gw for iface %s, result %d\n",
							strIface, nresult);
		}

		nresult = doRoute(hSocket, TRUE, strIface, zeroAddr, gwAddr, zeroAddr);
		closeSocket(hSocket);
	}

	return nresult;
}

/*******************************************************************************
 * Debugging support
 */

void CNetRoute::dump(const char* strPref) const
{
	CFile		procFile;
	char		strLine[256], *s;
	const char*	el;
	char*		svptr;
	size_t		length;
	result_t	nresult;

	log_dump(">> %sKernel IP Routing table:\n", strPref);
	log_dump("   Destination     Gateway         Genmask             Iface\n");

	nresult = procFile.open("/proc/net/route", CFile::fileRead);
	if ( nresult == ESUCCESS )  {
		while ( TRUE  )  {
			CString		strIface;
			uint32_t	dest, gw, mask;

			/* Iface Destination Gateway Flags RefCnt Use Metric Mask MTU Window IRTT */

			length = sizeof(strLine);
			nresult = procFile.readLine(strLine, &length, "\n");
			if ( nresult != ESUCCESS || length == 0 )  {
				break;
			}

			s = strLine;
			rTrimEol(s); rTrimWs(s);
			lTrim(s, " \t");

			/* Iface name */
			el = strtok_r(s, " \t", &svptr);
			if ( el == NULL ) { break; }
			strIface = el;

			/* Destination address */
			el = strtok_r(NULL, " \t", &svptr);
			if ( el == NULL ) { break; }
			if ( CString(el).getNumber(dest, 16) != ESUCCESS )  { continue; }

			/* Gateway */
			el = strtok_r(NULL, " \t", &svptr);
			if ( el == NULL ) { break; }
			if ( CString(el).getNumber(gw, 16) != ESUCCESS )  { break; }

			/* Skip: Flags, RefCnt, Use, Metric */
			el = strtok_r(NULL, " \t", &svptr);
			if ( el == NULL ) { break; }
			el = strtok_r(NULL, " \t", &svptr);
			if ( el == NULL ) { break; }
			el = strtok_r(NULL, " \t", &svptr);
			if ( el == NULL ) { break; }
			el = strtok_r(NULL, " \t", &svptr);
			if ( el == NULL ) { break; }

			/* Mask */
			el = strtok_r(NULL, " \t", &svptr);
			if ( el == NULL ) { break; }
			if ( CString(el).getNumber(mask, 16) != ESUCCESS )  { break; }

			log_dump("   %-16s%-16s%-20s%-16s\n",
					 	CNetHost((ip_addr_t)dest).cs(),
					 	CNetHost((ip_addr_t)gw).cs(),
						CNetHost((ip_addr_t)mask).cs(),
					 	strIface.cs());
		}

		procFile.close();
	}
}
