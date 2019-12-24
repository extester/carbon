/*
 *  Carbon/Contact module
 *  Asynchronous DNS client service
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.06.2018 12:51:02
 *      Initial revision.
 */

#include "contact/dns_client_service.h"

#define MODULE_NAME		"dns-client-service"

static CDnsClientService* g_pDnsClientService = 0;

/*******************************************************************************
 * CDnsClientService class
 */

CDnsClientService::CDnsClientService() :
	CModule(MODULE_NAME),
	CEventLoopThread(MODULE_NAME),
	CEventReceiver(this, MODULE_NAME)
{
}

CDnsClientService::~CDnsClientService()
{
}

void CDnsClientService::sendReply(result_t nresult, ip_addr_t hostIp,
								  seqnum_t nSessId, CEventReceiver* pReplyReceiver)
{
	if ( pReplyReceiver != 0 )  {
		CEvent*		pEvent;

		pEvent = new CEvent(EV_DNS_REPLY, pReplyReceiver, (PPARAM)(unatural_t)hostIp, (NPARAM)nresult, "dns-reply");
		pEvent->setSessId(nSessId);
		appSendEvent(pEvent);
	}
}

void CDnsClientService::doResolve(const dns_request_data_t* pData, seqnum_t nSessId)
{
	struct in_addr 	inAddr;
	const char*		dns0 = pData->dns[0].isValid() ? pData->dns[0].cs() : "";
	const char*		dns1 = pData->dns[1].isValid() ? pData->dns[1].cs() : "";
	result_t 		nresult;

	log_debug(L_DNS_CLIENT_FL, "[dsn_cli_serv] resolving name '%s' on servers: '%s', '%s'\n",
			  	pData->strName.cs(), dns0, dns1);

	nresult = dnsLookup(pData->strName, pData->hrTimeout, dns0, dns1, &inAddr);
	sendReply(nresult, nresult == ESUCCESS ? inAddr.s_addr : 0, nSessId, pData->pReplyReceiver);
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
boolean_t CDnsClientService::processEvent(CEvent* pEvent)
{
	CEventDnsRequest*	pEventReq;
	boolean_t			bProcessed = FALSE;

	switch (pEvent->getType()) {
		case EV_DNS_RESOLVE:
			pEventReq = dynamic_cast<CEventDnsRequest*>(pEvent);
			shell_assert(pEventReq);
			if ( pEventReq )  {
				doResolve(pEventReq->getData(), pEvent->getSessId());
			}

			bProcessed = TRUE;
			break;
	}

	return bProcessed;
}

result_t CDnsClientService::init()
{
	result_t	nresult;

	nresult = CModule::init();
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	nresult = CEventLoopThread::start();
	if ( nresult != ESUCCESS )  {
		CModule::terminate();
		return nresult;
	}

	return nresult;
}

void CDnsClientService::terminate()
{
	CEventLoopThread::stop();
	CModule::terminate();
}

/*******************************************************************************
 * Debugging support
 */

void CDnsClientService::dump(const char* strPref) const
{
}

/*******************************************************************************/

CDnsClientService* getDsnClientService()
{
	return g_pDnsClientService;
}

result_t initDnsClientService()
{
	result_t	 nresult = ESUCCESS;

	if ( g_pDnsClientService == 0 ) {
		g_pDnsClientService = new CDnsClientService();
		nresult = g_pDnsClientService->init();
		if ( nresult == ESUCCESS ) {
			log_debug(L_DNS_CLIENT_FL, "[dns_cli_serv] DNS client service started\n");
		}
		else {
			log_error(L_DNS_CLIENT, "[dns_cli_serv] CAN'T START DNS CLIENT SERVICE, RESULT %d\n", nresult);
			SAFE_DELETE(g_pDnsClientService);
		}
	}

	return nresult;
}

void terminateDnsClientService()
{
	if ( g_pDnsClientService )  {
		g_pDnsClientService->terminate();
		SAFE_DELETE(g_pDnsClientService);
		log_debug(L_DNS_CLIENT_FL, "[dns_cli_serv] DNS client service terminated\n");
	}
}
