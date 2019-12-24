/*
 *  Carbon/Contact module
 *  Asynchronous DNS service
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.06.2018 12:04:28
 *      Initial revision.
 */

#ifndef __CONTACT_DNS_CLIENT_H_INCLUDED__
#define __CONTACT_DNS_CLIENT_H_INCLUDED__

#include "shell/netaddr.h"

#include "carbon/cstring.h"
#include "carbon/event.h"

struct dns_request_data_t {
	CString 		strName;
	hr_time_t		hrTimeout;
	CNetHost		dns[2];

	CEventReceiver*	pReplyReceiver;
};

typedef CEventT<dns_request_data_t, EV_DNS_RESOLVE>	CEventDnsRequest;

interface IDnsClientResult {
	public:
		virtual void dnsRequestResult(const CNetHost& ip, result_t nresult) = 0;
};

class CDnsClient : public CEventReceiver
{
	protected:
		IDnsClientResult*		m_pParent;
		seqnum_t				m_nSessId;

	public:
		CDnsClient(IDnsClientResult* pParent, CEventLoop* pOwner = 0);
		virtual ~CDnsClient();

	public:
		virtual void resolve(const char* strDnsName, hr_time_t hrTimeout,
			const CNetHost& dnsServ0, const CNetHost& dnsServ1 = NETHOST_NULL);

		virtual void cancel();

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);
		virtual void notify(result_t nresult, ip_addr_t ipAddr);
};

#endif /* __CONTACT_DNS_CLIENT_H_INCLUDED__ */

