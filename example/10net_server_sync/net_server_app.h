/*
 *	Carbon Framework Examples
 *	Network server synchronous I/O
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 05.09.2016 17:28:22
 *	    Initial revision.
 */

#ifndef __EXAMPLE_NET_SERVER_APP_H_INCLUDED__
#define __EXAMPLE_NET_SERVER_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/net_server/net_server.h"

class CNetServerApp : public CApplication
{
    protected:
		CNetServer			m_netServer;		/* NetServer object */
		net_connection_t	m_hConnection;		/* Connection handler or 0 */

    public:
		CNetServerApp(int argc, char* argv[]);
        virtual ~CNetServerApp();

	public:
        virtual result_t init();
        virtual void terminate();

	protected:
		virtual void initLogger();
		virtual boolean_t processEvent(CEvent* pEvent);

	private:
		void disconnect();
};


#endif /* __EXAMPLE_NET_SERVER_APP_H_INCLUDED__ */
