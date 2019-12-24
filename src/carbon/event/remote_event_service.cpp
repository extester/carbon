/*
 *  Carbon framework
 *  External events I/O service
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.08.2016 14:39:18
 *      Initial revision.
 */

#include "shell/shell.h"
#include "shell/file.h"

#include "carbon/utils.h"
#include "carbon/net_container.h"
#include "carbon/event/remote_event_service.h"

#define MODULE_NAME			"remote_event_service"


/*******************************************************************************
 * Class CRemoteEventService
 */

CRemoteEventService::CRemoteEventService(const char* strSelfRid, CEventReceiver* pParent) :
	CModule(MODULE_NAME),
	m_pParent(pParent),
	m_pNetConnector(0),
	m_strSelfRid(strSelfRid)
{
	dec_ptr<CRemoteEvent>	pEvent = new CRemoteEvent;
	m_pNetConnector = new CTcpConnector(new CRemoteEventContainer(pEvent),
										pParent, REMOTE_EVENT_MAX_CONNECTIONS);
}

CRemoteEventService::~CRemoteEventService()
{
	SAFE_DELETE(m_pNetConnector);
}

/*
 * Validate a remote events root path and remove an existing
 * listening socket
 *
 * Return: ESUCCESS, ...
 */
result_t CRemoteEventService::preparePath()
{
	int 		n = 0;
	result_t	nresult = ESUCCESS;

	m_strSocket = REMOTE_EVENT_ROOT_PATH;
	m_strSocket.appendPath(m_strSelfRid);

	CFile::makeDir(REMOTE_EVENT_ROOT_PATH);
	CFile::removeFile(m_strSocket);

	while ( CFile::fileExists(m_strSocket) && n < 10 )  {
		sleep_ms(100);
		n++;
	}

	if ( CFile::fileExists(m_strSocket) )  {
		log_error(L_GEN, "[remote_event_service] failed to prepare socket %s\n",
				  m_strSocket.cs());
		nresult = EEXIST;
	}

	return nresult;

}

result_t CRemoteEventService::sendEvent(CRemoteEvent* pEvent, const char* strDestRid,
								CEventReceiver* pReceiver, seqnum_t nSessId)
{
	dec_ptr<CRemoteEventContainer>	pContainer = new CRemoteEventContainer(pEvent);
	CString							strSocket;
	result_t						nresult;

	strSocket = REMOTE_EVENT_ROOT_PATH;
	strSocket.appendPath(strDestRid);

	pEvent->setReplyRid(m_strSelfRid);
	nresult = m_pNetConnector->send(pContainer, strSocket, pReceiver, nSessId);

	pEvent->release();
	return nresult;
}

/*
boolean_t CRemoteEventService::processEvent(CEvent* pEvent)
{
	CEventPacket*			pEventPacket;
	CRemoteEventContainer*	pContainer;
	boolean_t				bProcessed = FALSE;

	switch ( pEvent->getType() )  {
		case EV_NETCONN_PACKET:
			pEventPacket = dynamic_cast<CEventPacket*>(pEvent);
			shell_assert(pEventPacket);
			if ( pEventPacket )  {
				pContainer = dynamic_cast<CRemoteEventContainer*>(pEventPacket->getContainer());
				if ( pContainer != 0 )  {
					dec_ptr<CRemoteEvent>	pRemoteEvent = pContainer->getEvent();
					pRemoteEvent->setReceiver(pRemoteEvent->getRealReceiver());
					pRemoteEvent->reference();
					appSendEvent(pRemoteEvent);
				}
				else {
					log_debug(L_GEN, "[remote_event_service] received a packet with unsupported "
							  "container type, dropped\n");
				}
			}

			bProcessed = TRUE;
			break;
	}

	return bProcessed;
}
*/

result_t CRemoteEventService::init()
{
	dec_ptr<CRemoteEvent>	pEvent = new CRemoteEvent;
	result_t	nresult;

	nresult = preparePath();
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	nresult = CModule::init();
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	nresult = m_pNetConnector->init();
	if ( nresult != ESUCCESS )  {
		CModule::terminate();
		return nresult;
	}

	nresult = m_pNetConnector->startListen(m_strSocket);
	if ( nresult != ESUCCESS )  {
		m_pNetConnector->terminate();
		CModule::terminate();
	}

	return nresult;
}

void CRemoteEventService::terminate()
{
	m_pNetConnector->terminate();
	CModule::terminate();
	CFile::removeFile(m_strSocket);
}

/*******************************************************************************
 * Debugging support
 */

#if CARBON_DEBUG_DUMP

void CRemoteEventService::dump(const char* strPref) const
{
	UNUSED(strPref);
}

#endif /* CARBON_DEBUG_DUMP */

/*******************************************************************************/

CRemoteEventService* g_pRemoteEventService = 0;

result_t initRemoteEventService(const char* ridSelf, CEventReceiver* pParent)
{
	result_t	nresult = ESUCCESS;

	if ( g_pRemoteEventService == 0 ) {
		log_info(L_BOOT, "[service] initialising Remote Event Service\n");

		g_pRemoteEventService = new CRemoteEventService(ridSelf, pParent);
		nresult = g_pRemoteEventService->init();
		if ( nresult != ESUCCESS )  {
			log_error(L_GEN, "[service] failed to init Remote Event Service, result: %d\n",
					  	nresult);
			delete g_pRemoteEventService;
			g_pRemoteEventService = 0;
		}
	}

	return nresult;
}

void exitRemoteEventService()
{
	if ( g_pRemoteEventService ) {
		log_info(L_BOOT, "[service] terminating Remote Event Service\n");
		g_pRemoteEventService->terminate();
		delete g_pRemoteEventService;
		g_pRemoteEventService = 0;
	}
}
