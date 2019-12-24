/*
 *  Carbon framework
 *  Asynchronous DNS client service
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.06.2018 12:36:54
 *      Initial revision.
 */

#ifndef __CONTACT_DNS_CLIENT_SERVICE_H_INCLUDED__
#define __CONTACT_DNS_CLIENT_SERVICE_H_INCLUDED__

#include "shell/netaddr.h"

#include "carbon/cstring.h"
#include "carbon/event.h"
#include "carbon/event/eventloop.h"
#include "carbon/module.h"

#include "contact/dns.h"
#include "contact/dns_client.h"

class CDnsClientService : public CModule, public CEventLoopThread, public CEventReceiver
{
    protected:
		CMultiDns			m_resolver;		/* DNS Resolver */

    public:
		CDnsClientService();
        virtual ~CDnsClientService();

	public:
		virtual result_t init();
		virtual void terminate();

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);

		void doResolve(const dns_request_data_t* pData, seqnum_t nSessId);
		void sendReply(result_t nresult, ip_addr_t hostIp, seqnum_t nSessId,
					   CEventReceiver* pReplyReceiver);
};

extern CDnsClientService* getDsnClientService();
extern result_t initDnsClientService();
extern void terminateDnsClientService();

#endif /* __CONTACT_DNS_CLIENT_SERVICE_H_INCLUDED__ */
