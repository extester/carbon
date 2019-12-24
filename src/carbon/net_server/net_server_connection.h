/*
 *  Carbon framework
 *  Network server single I/O class
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 23.06.2016 17:16:52
 *      Initial revision.
 */

#ifndef __CARBON_NET_SERVER_CONNECTION_H_INCLUDED__
#define __CARBON_NET_SERVER_CONNECTION_H_INCLUDED__

#include "shell/socket.h"

#include "carbon/net_container.h"
#include "carbon/event/eventloop.h"

class CNetServerConnection;
class CNetServer;

typedef CNetServerConnection*       net_connection_t;
#define NET_CONNECTION_NULL			((net_connection_t)0)

class CNetServerConnection : public CEventLoopThread, public CEventReceiver
{
    protected:
        dec_ptr<CSocketRef>		m_pSocket;
		CNetServer*				m_pParent;
		boolean_t 				m_bRecvBreak;
		boolean_t				m_bFailSent;

    public:
        CNetServerConnection(CSocketRef* pSocket, CNetServer* pParent);
        virtual ~CNetServerConnection();

	public:
		virtual result_t init();
		virtual void terminate();

		virtual void abort();
		boolean_t isConnected() const {
			return m_pSocket->isOpen();		/* TODO FIX */
		}

		virtual void disconnect();

		net_connection_t getHandle() { return this; }

	protected:
        virtual boolean_t processEvent(CEvent* pEvent);
		virtual void notify();
		virtual void runEventLoop();

		virtual void doSend(CNetContainer* pContainer, seqnum_t nSessId);
		virtual result_t doRecv(hr_time_t htTimeout);
		virtual void notifyClosed();
		virtual void notifySend(result_t nOpResult, seqnum_t nSessId);
		virtual void notifyRecv(CNetContainer* pContainer);
		virtual CNetContainer* getRecvTemplRef();

	private:
		result_t waitReceive();
		result_t processReceive();

};

#endif  /* __CARBON_NET_SERVER_CONNECTION_H_INCLUDED__ */
