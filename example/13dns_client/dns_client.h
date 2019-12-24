/*
 *	Carbon Framework Examples
 *	DNS Client example
 *
 *  Copyright (c) 2017-2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 25.08.2017 11:43:43
 *	    Initial revision.
 *
 *	Revision 2.0, 05.06.2018 15:56:31
 *		Rewrote to user new DNS Client Service class
 */
/*
 * NOTE: Required compile-time option: CARBON_UDNS=1
 *
 * By default the resolver uses system-wide (/etc/resolv.conf) DNS servers,
 * uncomment GOOGLE_DNS macro to use google DNS servers (8.8.8.8, 8.8.4.4)
 */

#ifndef __EXAMPLE_DNS_CLIENT_H_INCLUDED__
#define __EXAMPLE_DNS_CLIENT_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"

#include "contact/dns_client.h"

#define GOOGLE_DNS				1

class CDnsResolveApp : public CApplication, public IDnsClientResult
{
    private:
		CDnsClient			m_client;		/* Resolver object */

		static const char*	m_arHost[];		/* Hosts to resolve */
		size_t				m_nIndex;		/* Current host index */

    public:
		CDnsResolveApp(int argc, char* argv[]);
        virtual ~CDnsResolveApp();

	public:
        virtual result_t init();
        virtual void terminate();

    private:
        virtual boolean_t processEvent(CEvent* pEvent);
		virtual void dnsRequestResult(const CNetHost& ip, result_t nresult);
};


#endif /* __EXAMPLE_DNS_CLIENT_H_INCLUDED__ */
