/*
 *	Carbon Framework Examples
 *	Synchronous Network server client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 31.08.2016 17:14:14
 *	    Initial revision.
 */

#ifndef __EXAMPLE_NET_SYNC_CLIENT_APP_H_INCLUDED__
#define __EXAMPLE_NET_SYNC_CLIENT_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/timer.h"
#include "carbon/net_connector/tcp_connector.h"

class CNetSyncClientApp : public CApplication
{
    protected:
		CTcpConnector		m_netConnector;		/* Network connector */
		CTimer*				m_pTimer;

    public:
		CNetSyncClientApp(int argc, char* argv[]);
        virtual ~CNetSyncClientApp();

	public:
        virtual result_t init();
        virtual void terminate();

	private:
		void timerHandler(void* pData);
};


#endif /* __EXAMPLE_NET_SYNC_CLIENT_APP_H_INCLUDED__ */
