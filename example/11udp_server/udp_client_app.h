/*
 *	Carbon Framework Examples
 *	UDP network I/O client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 07.08.2016 18:35:56
 *	    Initial revision.
 */

#ifndef __EXAMPLE_UDP_CLIENT_APP_H_INCLUDED__
#define __EXAMPLE_UDP_CLIENT_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/timer.h"
#include "carbon/application.h"
#include "carbon/net_connector/udp_connector.h"

class CUdpClientApp : public CApplication
{
    protected:
		CUdpConnector		m_netConnector;		/* Network connector */
		CTimer*				m_pTimer;

    public:
		CUdpClientApp(int argc, char* argv[]);
        virtual ~CUdpClientApp();

	public:
        virtual result_t init();
        virtual void terminate();

    protected:
		virtual void initLogger();

	private:
		void sendMessage();

		void stopTimer();
		void timerHandler(void* pData);
};

#endif /* __EXAMPLE_UDP_CLIENT_APP_H_INCLUDED__ */
