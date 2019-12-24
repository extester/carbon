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
 *  Revision 1.0, 26.09.2017 14:45:12
 *      Initial revision.
 *
 *  Revision 1.1, 16.05.2018 17:54:42
 *  	Added addRoute:2, delRoute:2, dump().
 */

#ifndef __CONTACT_NET_ROUTE_H_INCLUDED__
#define __CONTACT_NET_ROUTE_H_INCLUDED__

#include "shell/shell.h"
#include "shell/netaddr.h"

#include "carbon/logger.h"
#include "carbon/module.h"

class CNetRoute : public CModule
{
	public:
		CNetRoute();
		virtual ~CNetRoute();

	public:
		result_t addRoute(const char* strIface, const CNetHost& destAddr,
						  const CNetHost& gwAddr, const CNetHost& destMask);
		result_t addRoute(const char* strIface, const CNetHost& destAddr);

		result_t delRoute(const char* strIface, const CNetHost& destAddr,
						  const CNetHost& gwAddr, const CNetHost& destMask);
		result_t delRoute(const char* strIface, const CNetHost& destAddr);

		result_t setDefaultGateway(const char* strIface, const CNetHost& gwAddr);

		virtual void dump(const char* strPref = "") const;

	protected:
		result_t openSocket(int& hSocket) const;
		void closeSocket(int hSocket) const;

		result_t doRoute(int hSocket, boolean_t bAdd, const char* strIface,
				const CNetHost& destAddr, const CNetHost& gwAddr, const CNetHost& destMask);

		result_t delDefaultGateway(int hSocket);
};

#endif /* __CONTACT_NET_ROUTE_H_INCLUDED__ */
