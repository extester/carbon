/*
 *  Carbon framework
 *  Network server module
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 23.06.2016 13:21:59
 *      Initial revision.
 */
/*
 * Purpose:
 *      Synchronous/Asynchronous network server with permanent connections
 *
 * Constructor:
 *      CNetServer(size_t nMaxConnection, CNetContainer* pRecvTempl, CEventReceiver* pParent);
 *      	nMaxConnection	maximum simultaneous connections
 * 			pRecvTempl		sample container for receiving the new containers
 *      	pParent			default container receiver
 *
 *
 * I) Initialisation/Deinitialisation API:
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * 		result_t init();
 * 			Server initialisation;
 *
 * 		void terminate();
 * 			Server termination.
 * 			Listening thread is stopped.
 * 			All client connections are stopped as well.
 *
 * 		result_t startListen(const CNetAddr& listenAddr);
 * 			Starting listen on the given network address.
 * 			listenAddr		address to listen on
 *
 * 		void stopListen();
 * 			Stop listening if any.
 *
 * 		void closeConnections();
 * 			Close all client connections and delete clients if any.
 *
 *
 * II) Settings API:
 *     ~~~~~~~~~~~~~
 *
 *      void setSendTimeout(hr_time_t hrTimeout);
 *          Setup send maximum timeout.
 *
 *		hr_time_t getSendTimeout() const;
 *			Get current send timeout.
 *
 *
 * III) Public API:
 *      ~~~~~~~~~~~
 *
 *		void send(CNetContainer* pContainer, net_connection_t hConnection, seqnum_t sessId = NO_SEQNUM);
 *          Send a container through an open connection.
 *          pContainer		container to send
 *          hConnection		open connection
 *          sessId			unique session Id (NO_SEQNUM - do not send reply)
 *
 *		result_t sendSync(CNetContainer* pContainer, net_connection_t hConnection);
 *          Send a container through an open connection in syncronous mode.
 *          pContainer		container to send
 *          hConnection		open connection
 *
 * 		result_t disconnect(net_connection_t hConnection);
 * 			Disconnect open connection;
 *
 * 		result_t isConnected(net_connection_t hConnection);
 * 			Return: ESUCCESS if a given connection is open, return ENOTCONN if
 * 			a given connection is closed, EINVAL if no such connection found.
 *
 *
 * IV) Notification API:
 *     ~~~~~~~~~~~~~~~~~
 *
 *     EV_NET_SERVER_DISCONNECTED
 *     Send a message to the parent if a connection was closed by remote peer.
 *     PPARAM - client handle
 *
 */

#ifndef __CARBON_NET_SERVER_H_INCLUDED__
#define __CARBON_NET_SERVER_H_INCLUDED__

#include <vector>

#include "shell/socket.h"

#include "carbon/net_container.h"
#include "carbon/module.h"
#include "carbon/tcp_server.h"
#include "carbon/event.h"
#include "carbon/thread.h"
#include "carbon/net_server/net_server_connection.h"

#define NET_SERVER_SEND_TIMEOUT         HR_4SEC
#define NET_SERVER_RECV_TIMEOUT         HR_16SEC

/*
 * Received a new container event
 *
 * 		pReceiver			notify receiver
 * 		pContainer			received container
 * 		hConnection			connection handle
 */
class CEventNetServerRecv : public CEvent
{
	public:
		CEventNetServerRecv(CEventReceiver* pReceiver, CNetContainer* pContainer,
							net_connection_t hConnection) :
			CEvent(EV_NET_SERVER_RECV, pReceiver, (PPARAM)pContainer,
			   		(NPARAM)hConnection, "net_serv_recv")
		{
			SAFE_REFERENCE(pContainer);
		}

		virtual ~CEventNetServerRecv()
		{
			CNetContainer*	pContainer = getContainer();
			SAFE_RELEASE(pContainer);
		}

	public:
		CNetContainer* getContainer() const
		{
			return static_cast<CNetContainer*>(getpParam());
		}

		net_connection_t getHandle() const
		{
			return reinterpret_cast<net_connection_t>(getnParam());
		}
};

class CEventNetServerDoSend : public CEvent
{
	public:
		CEventNetServerDoSend(CNetContainer* pContainer, CEventReceiver* pReceiver,
							  seqnum_t sessId) :
			CEvent(EV_NET_SERVER_DO_SEND, pReceiver, (PPARAM)pContainer,
			   		0, "net_serv_do_send")
		{
			setSessId(sessId);
			SAFE_REFERENCE(pContainer);
		}

		virtual ~CEventNetServerDoSend()
		{
			CNetContainer*	pContainer = getContainer();
			SAFE_RELEASE(pContainer);
		}

	public:
		CNetContainer* getContainer() const {
			return static_cast<CNetContainer*>(getpParam());
		}
};


/*
 * NetServer module statistic data
 */
typedef struct
{
	counter_t   client;   		/* Accepted clients */
	counter_t   client_fail;	/* Accepted client internal errors */

	counter_t   recv;           /* Receive requests */
	counter_t   send;           /* Send requests */
	counter_t   fail;        	/* I/O fails */

	counter_t   connection;		/* Current open connection count */
} __attribute__ ((packed)) netserv_stat_t;


/*
 * CNetServer
 */
class CNetServer : public CModule, public CTcpServer
{
    protected:
        CEventReceiver*     m_pParent;          /* Default reply receiver */
        netserv_stat_t      m_stat;             /* Module statistic */

        CThread             m_listenThread;		/* Listening in the separate thread */

		std::vector<CNetServerConnection*>	m_arConnection;
        size_t              m_nMaxConnection;	/* Maximum simultaneous connections */
		mutable CMutex		m_lock;				/* Synchronisation */

		dec_ptr<CNetContainer>	m_pRecvTempl;	/* Recv container, access under lock */
		hr_time_t           m_hrSendTimeout;    /* Send timeout, access under lock */
		hr_time_t			m_hrRecvTimeout;	/* Receive timeout, access under lock */

    public:
        CNetServer(size_t nMaxConnection, CNetContainer* pRecvTempl, CEventReceiver* pParent);
        virtual ~CNetServer();

    public:
        CEventReceiver* getReceiver() const {
            return m_pParent;
        }

		CNetContainer* getRecvTemplRef() {
			CAutoLock locker(m_lock);
			SAFE_REFERENCE((CNetContainer*)m_pRecvTempl);
			return m_pRecvTempl;
		}

        /* Statistic functions */
        virtual size_t getStatSize() const {
            return sizeof(m_stat);
        }

        virtual void getStat(void* pBuffer, size_t nSize) const {
            size_t rsize = MIN(nSize, sizeof(m_stat));
            UNALIGNED_MEMCPY(pBuffer, &m_stat, rsize);
        }

        virtual void resetStat();
		virtual void dump(const char* strPref = "") const;

        void statClient()       { counter_inc(m_stat.client); }
        void statClientFail()   { counter_inc(m_stat.client_fail); }
        void statRecv()         { counter_inc(m_stat.recv); }
        void statSend()         { counter_inc(m_stat.send); }
        void statFail()			{ counter_inc(m_stat.fail); }
		void statConnection(size_t count)  { counter_set(m_stat.connection, count); }

        virtual result_t init();
        virtual void terminate();

        /*
         * Public API
         */
        virtual result_t startListen(const CNetAddr& listenAddr);
        virtual void stopListen();

		virtual void closeConnections();

		virtual result_t disconnect(net_connection_t hConnection);
		virtual result_t isConnected(net_connection_t hConnection);

		virtual result_t send(CNetContainer* pContainer, net_connection_t hConnection,
							  seqnum_t sessId = NO_SEQNUM);
		virtual result_t sendSync(CNetContainer* pContainer, net_connection_t hConnection);

        void setSendTimeout(hr_time_t hrTimeout)
		{
			CAutoLock locker(m_lock);
			m_hrSendTimeout = hrTimeout;
        }

		void setRecvTimeout(hr_time_t hrTimeout)
		{
			CAutoLock locker(m_lock);
			m_hrRecvTimeout = hrTimeout;
		}

        hr_time_t getSendTimeout() const
		{
			CAutoLock locker(m_lock);
			return m_hrSendTimeout;
        }

		hr_time_t getRecvTimeout() const
		{
			CAutoLock locker(m_lock);
			return m_hrRecvTimeout;
		}

	protected:
		virtual result_t processClient(CSocketRef* pSocket);

        virtual int findConnection(CNetServerConnection* pConnection) const;
		virtual CNetServerConnection* createConnection(CSocketRef* pSocket);
		virtual result_t deleteConnection(CNetServerConnection* pConnection);

	private:
        void* threadListen(CThread* pThread, void* pData);
};

#endif  /* __CARBON_NET_SERVER_H_INCLUDED__ */
