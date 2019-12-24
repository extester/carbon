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

#ifndef __CARBON_EVENT_REMOTE_EVENT_SERVICE_H_INCLUDED__
#define __CARBON_EVENT_REMOTE_EVENT_SERVICE_H_INCLUDED__

#include "shell/config.h"

#include "carbon/module.h"
#include "carbon/event/remote_event.h"
#include "carbon/cstring.h"
#include "carbon/net_connector/tcp_connector.h"

#ifndef REMOTE_EVENT_ROOT_PATH
#define REMOTE_EVENT_ROOT_PATH			"/tmp"
#endif /* REMOTE_EVENT_ROOT_PATH */

#ifndef REMOTE_EVENT_MAX_CONNECTIONS
#define REMOTE_EVENT_MAX_CONNECTIONS	100
#endif /* REMOTE_EVENT_MAX_CONNECTIONS */

class CRemoteEventService : public CModule
{
	protected:
		CEventReceiver*		m_pParent;

		CTcpConnector*		m_pNetConnector;
		CString				m_strSelfRid;
		CString				m_strSocket;

	public:
		CRemoteEventService(const char* strSelfRid, CEventReceiver* pParent);
		virtual ~CRemoteEventService();

	public:
		virtual result_t init();
		virtual void terminate();

		virtual result_t sendEvent(CRemoteEvent* pEvent, const char* strDestRid,
							CEventReceiver* pReceiver = 0, seqnum_t nSessId = NO_SEQNUM);

	private:
		result_t preparePath();

#if CARBON_DEBUG_DUMP
	public:
		virtual void dump(const char* strPref = "") const;
#endif /* CARBON_DEBUG_DUMP */
};

extern CRemoteEventService* g_pRemoteEventService;

extern result_t initRemoteEventService(const char* ridSelf, CEventReceiver* pParent);
extern void exitRemoteEventService();


#endif /* __CARBON_EVENT_REMOTE_EVENT_SERVICE_H_INCLUDED__ */

