/*
 *	Carbon Framework Examples
 *	Synchronous Network server
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 31.08.2016 17:15:02
 *	    Initial revision.
 */

#ifndef __EXAMPLE_NET_SYNC_SERVER_APP_H_INCLUDED__
#define __EXAMPLE_NET_SYNC_SERVER_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/net_connector/tcp_connector.h"

class CNetSyncServerApp : public CApplication
{
    protected:
		CTcpConnector		m_netConnector;		/* Network connector */

    public:
		CNetSyncServerApp(int argc, char* argv[]);
        virtual ~CNetSyncServerApp();

	public:
        virtual result_t init();
        virtual void terminate();

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);

	private:
		void sendReply(CEventTcpConnRecv* pEvent);
};


#endif /* __EXAMPLE_NET_SYNC_SERVER_APP_H_INCLUDED__ */
