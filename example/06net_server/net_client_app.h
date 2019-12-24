/*
 *	Carbon Framework Examples
 *	Network server client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 27.06.2016 11:30:17
 *	    Initial revision.
 */

#ifndef __EXAMPLE_NET_CLIENT_APP_H_INCLUDED__
#define __EXAMPLE_NET_CLIENT_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/net_server/net_client.h"

class CNetClientApp : public CApplication
{
    protected:
		CNetClient			m_netClient;	/* Netserver client */
		seqnum_t			m_sessId;		/* Unique session Id for Send/Recv */
		seqnum_t			m_sessIdIo;		/* Unique session Id for I/O */

    public:
		CNetClientApp(int argc, char* argv[]);
        virtual ~CNetClientApp();

	public:
        virtual result_t init();
        virtual void terminate();

    private:
        virtual boolean_t processEvent(CEvent* pEvent);
		void disconnect();
};


#endif /* __EXAMPLE_NET_CLIENT_APP_H_INCLUDED__ */