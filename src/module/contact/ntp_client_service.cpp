/*
 *  Carbon/Contact module
 *  Asynchronous NTP client service
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 20.06.2018 11:20:25
 *      Initial revision.
 */

#include "contact/ntp_client_service.h"

#define MODULE_NAME			"ntp-client-serv"

/*******************************************************************************
 * CNtpClientService class
 */

CNtpClientService::CNtpClientService(size_t nCount, hr_time_t hrTimeout) :
	CModule(MODULE_NAME),
	CEventLoopThread(MODULE_NAME),
	CEventReceiver(appMainLoop(), MODULE_NAME),
	m_ntpClient(nCount, hrTimeout)
{
	shell_assert(nCount > 0);
	shell_assert(hrTimeout != HR_0);
}

CNtpClientService::~CNtpClientService()
{
}

/*
 * Process request
 *
 * 		pInData			request data
 * 		nSessId			request Id
 *
 * Note: result returned by event to the specified reply receiver
 */
void CNtpClientService::doGetTime(const ntp_request_data_t* pInData, seqnum_t nSessId)
{
	ntp_reply_data_t	outData;
	result_t			nresult;

	nresult = m_ntpClient.getTime(pInData->ntpServer.cs(), &outData.hrTime, &outData.hrTimestamp);
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[ntp_client_serv] ntp client request failure, ntp server '%s', "
							"result %d\n", pInData->ntpServer.cs(), nresult);
	}

	if ( pInData->pReplyReceiver )  {
		CEventNtpReply*		pEvent;

		pEvent = new CEventNtpReply(&outData, pInData->pReplyReceiver, (PPARAM)0,
									(NPARAM)nresult, "ntp-serv");
		pEvent->setSessId(nSessId);
		appSendEvent(pEvent);
	}
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
boolean_t CNtpClientService::processEvent(CEvent* pEvent)
{
	CEventNtpRequest*	pEventReq;
	boolean_t			bProcessed = FALSE;

	switch (pEvent->getType()) {
		case EV_NTP_CLIENT_REQUEST:
			pEventReq = dynamic_cast<CEventNtpRequest*>(pEvent);
			shell_assert(pEventReq);
			if ( pEventReq )  {
				doGetTime(pEventReq->getData(), pEvent->getSessId());
			}

			bProcessed = TRUE;
			break;
	}

	return bProcessed;
}

/*
 * Initialise service
 *
 * Return: ESUCCESS, ...
 */
result_t CNtpClientService::init()
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

/*
 * Terminate service and release resources
 */
void CNtpClientService::terminate()
{
	CEventLoopThread::stop();
	CModule::terminate();
}

/*******************************************************************************
 * Debugging support
 */

void CNtpClientService::dump(const char* strPref) const
{
	shell_unused(strPref);
}
