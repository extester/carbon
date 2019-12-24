/*
 *	Carbon Framework Examples
 *	UDP network I/O server
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 08.09.2016 11:22:21
 *	    Initial revision.
 */

#ifndef __EXAMPLE_UDP_SERVER_APP_H_INCLUDED__
#define __EXAMPLE_UDP_SERVER_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/net_connector/udp_connector.h"

class CUdpServerApp : public CApplication
{
    protected:
		CUdpConnector		m_netConnector;		/* Network connector */

    public:
		CUdpServerApp(int argc, char* argv[]);
        virtual ~CUdpServerApp();

	public:
        virtual result_t init();
        virtual void terminate();

	protected:
		virtual void initLogger();
		virtual boolean_t processEvent(CEvent* pEvent);

	private:
		result_t startServer();
};


#endif /* __EXAMPLE_UDP_SERVER_APP_H_INCLUDED__ */
