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
 *  Revision 1.0, 05.06.2018 12:13:24
 *      Initial revision.
 */

#include "contact/dns_client.h"
#include "contact/dns_client_service.h"

/*******************************************************************************
 * CDnsClient class
 */

CDnsClient::CDnsClient(IDnsClientResult* pParent, CEventLoop* pOwner) :
	CEventReceiver(pOwner ? pOwner : appMainLoop(), "dns-client"),
	m_pParent(pParent),
	m_nSessId(NO_SEQNUM)
{
	shell_assert(pParent);
}

CDnsClient::~CDnsClient()
{
}

void CDnsClient::notify(result_t nresult, ip_addr_t ipAddr)
{
	m_pParent->dnsRequestResult(CNetHost(ipAddr), nresult);
}

/*
 * Event processor
 *
 *      pEvent      event object to process
 *
 * Return:
 *      TRUE        event processed
 *      FALSE       event is not processed
 */
boolean_t CDnsClient::processEvent(CEvent* pEvent)
{
	seqnum_t	nSessId;
	result_t	nresult;
	boolean_t	bProcessed = FALSE;

	switch (pEvent->getType()) {
		case EV_DNS_REPLY:
			nSessId = pEvent->getSessId();
			if ( m_nSessId == nSessId )  {
				nresult = (result_t)pEvent->getnParam();
				m_nSessId = NO_SEQNUM;

				notify(nresult, nresult == ESUCCESS ?
						(ip_addr_t)A(pEvent->getpParam()) : 0);
				bProcessed = TRUE;
			}
			break;
	}

	return bProcessed;
}

void CDnsClient::resolve(const char* strDnsName, hr_time_t hrTimeout,
					 const CNetHost& dnsServ0, const CNetHost& dnsServ1)
{
	dns_request_data_t		data;
	CEventDnsRequest*		pEvent;
	CDnsClientService*		pService = getDsnClientService();

	shell_assert(strDnsName);
	shell_assert(hrTimeout != HR_0);

	data.strName = strDnsName;
	data.hrTimeout = hrTimeout;
	data.dns[0] = dnsServ0;
	data.dns[1] = dnsServ1;
	data.pReplyReceiver = this;

	m_nSessId = getUniqueId();

	if ( pService ) {
		pEvent = new CEventDnsRequest(&data, pService, "dns-cli");
		pEvent->setSessId(m_nSessId);
		appSendEvent(pEvent);
	}
	else {
		log_error(L_DNS_CLIENT, "[dns_cli] dns service is not running, request of '%s' failed\n",
				  	strDnsName);
		notify(EFAULT, 0);
	}
}

void CDnsClient::cancel()
{
	/* TODO: cancel request on server */
	m_nSessId = NO_SEQNUM;
}
