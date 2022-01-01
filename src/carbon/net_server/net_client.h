/*
 *  Carbon framework
 *  Network client module
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 24.06.2016 21:18:12
 *      Initial revision.
 */
/*
 * Purpose:
 *      Synchronous/Asynchronous network client with permanent connections
 *
 * Constructor:
 *      CNetClient(CEventReceiver* pParent);
 *      	pParent		default reply receiver
 *
 *
 * I) Initialisation/Deinitialisation API:
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * 		result_t init();
 *          initialisation with default parameters
 *
 * 		void terminate();
 * 			Disconnect an open connection and deinitialise client.
 *
 *
 * II) Settings API:
 *     ~~~~~~~~~~~~~
 *
 *     	void setBindAddr(const CNetAddr& bindAddr);
 *     		Set bind address for the new sockets.
 *
 * 		void setConnectTimeout(hr_time_t hrTimeout);
 *			Set maximum connecting time.
 *
 *		hr_time_t getConnectTimeout() const;
 *			Get current connect timeout.
 *
 *		void setTimeouts(hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout);
 *          Set send/receive maximum times.
 *
 *      void getTimeouts(hr_time_t* phrSendTimeout, hr_time_t* phrRecvTimeout);
 *          Obtain current send/recv timeouts.
 *
 *
 * III) Public API:
 *      ~~~~~~~~~~~
 *
 *      void connect(const CNetAddr& netAddr, seqnum_t sessId, socket_type_t sockType = SOCKET_TYPE_STREAM);
 *      	Connect to the remote server with the given address.
 *
 *      	netAddr		remote address to connect to
 *      	sessId		unique session Id, may not be NO_SEQNUM
 *      	sockType	socket type, SOCKEY_TYPE_xxx
 *
 *          Result event: EV_NET_CLIENT_CONNECTED, CEvent(NPARAM=nresult)
 *
 *      void connect(const char* strSocket, seqnum_t sessId, socket_type_t sockType = SOCKET_TYPE_STREAM);
 *      	Connect to the remote server with the given UNIX file socket
 *
 *      	strSocket	full filename of the UNIX  socket to connect to
 *      	sessId		unique session Id, may not be NO_SEQNUM
 *      	sockType	socket type, SOCKEY_TYPE_xxx
 *
 *          Result event: EV_NET_CLIENT_CONNECTED, CEvent(NPARAM=nresult)
 *
 * 		result_t connectSync(const CNetAddr& netAddr, socket_type_t sockType = SOCKET_TYPE_STREAM);
 * 			Connect to the remove host in synchronous mode.
 *
 *      	netAddr		remote address to connect to
 *      	sockType	socket type, SOCKEY_TYPE_xxx
 *
 *		void disconnect(seqnum_t sessId);
 *			Disconnect from the remote server.
 *
 *      	sessId		unique session Id, NO_SEQNUM: no result notify
 *
 *          Result event: EV_NET_CLIENT_DISCONNECTED, CEvent(NPARAM=nresult)
 *
 *		result_t disconnectSync();
 *			Disconnect from the remote server in synchronous mode.
 *
 *		boolean_t isConnected() const;
 * 			Return TRUE if a cleint is connected, return FALSE otherwise.
 *
 *		void send(CNetContainer* pContainer, seqnum_t sessId);
 *			Send a given container to the connected remote server.
 *
 *          pContainer		container to send
 *          sessId			unique session Id (NO_SEQNUM - do not send reply)
 *
 *		result_t sendSync(CNetContainer* pContainer);
 *			Send a given container to the connected remote server in synchronous mode.
 *
 *          pContainer		container to send
 *
 *      void recv(CNetContainer* pContainer, seqnum_t sessId);
 *          Receive a container with previously set timeout using session sessId
 *
 *          pContainer		container to receive a data
 *          sessId			unique session Id (may not be NO_SEQNUM)
 *
 *      result_t recvSync(CNetContainer* pContainer);
 *      	Receive a container with predefined timeout.
 *
 *      	pContainer		container to receive a data
 *
 *      void io(CNetContainer* pContainer, seqnum_t sessId)
 *          Send a container pContainer with previously set timeout, when receive
 *          a container (same container) with the previously set timeout using
 *          session sessId.
 *
 *          pContainer		container to send/receive
 *          sessId			unique session Id (may not be NO_SEQNUM)
 *
 *          Result event: EV_NET_CLIENT_RECV, CEventNetClientRecv(NPARAM=nresult)
 *
 * 		result_t ioSync(CNetContainer* pContainer);
 *          Send a container pContainer with previously set timeout, when receive
 *          a container (same container) with the previously set timeout in
 *          syncronous mode.
 *
 *      	pContainer		container to send/receive a data
 *
 * 		void cancel();
 * 			Stop current I/O operation and remove any pending I/O.
 */

#ifndef __CARBON_NET_CLIENT_H_INCLUDED__
#define __CARBON_NET_CLIENT_H_INCLUDED__

#include "shell/socket.h"
#include "shell/ssl_socket.h"

#include "carbon/cstring.h"
#include "carbon/net_container.h"
#include "carbon/lock.h"
#include "carbon/event/eventloop.h"
#include "carbon/module.h"
#include "carbon/event.h"


typedef struct {
	CNetAddr 		netAddr;
	CString			strSocket;
	socket_type_t	sockType;
} net_client_do_connect_t;

typedef CEventT<net_client_do_connect_t, EV_NET_CLIENT_DO_CONNECT>	CEventNetClientDoConnect;

class CEventNetClientDoSendBase : public CEvent
{
	public:
		CEventNetClientDoSendBase(CNetContainer* pContainer, CEventReceiver* pReceiver,
								  seqnum_t sessId, event_type_t eventType, const char* strDesc) :
			CEvent(eventType, pReceiver, (PPARAM)pContainer, 0, strDesc)
		{
			setSessId(sessId);
			shell_assert(pContainer);
			if ( pContainer ) pContainer->reference();
		}

		virtual ~CEventNetClientDoSendBase()
		{
			CNetContainer*	pContainer = getContainer();
			SAFE_RELEASE(pContainer);
		}

	public:
		CNetContainer* getContainer() const {
			return static_cast<CNetContainer*>(getpParam());
		}
};


class CEventNetClientDoSend : public CEventNetClientDoSendBase
{
	public:
		CEventNetClientDoSend(CNetContainer* pContainer, CEventReceiver* pReceiver,
							  seqnum_t sessId) :
			CEventNetClientDoSendBase(pContainer, pReceiver, sessId,
									  EV_NET_CLIENT_DO_SEND, "net_client_do_send")
		{
		}

		virtual ~CEventNetClientDoSend()
		{
		}
};

class CEventNetClientDoIo : public CEventNetClientDoSendBase
{
	public:
		CEventNetClientDoIo(CNetContainer* pContainer, CEventReceiver* pReceiver,
							seqnum_t sessId) :
			CEventNetClientDoSendBase(pContainer, pReceiver, sessId,
								  EV_NET_CLIENT_DO_IO, "net_client_do_io")
		{
		}

		virtual ~CEventNetClientDoIo()
		{
		}
};

class CEventNetClientDoRecv : public CEvent
{
	public:
		CEventNetClientDoRecv(CNetContainer* pContainer,
									CEventReceiver* pReceiver, seqnum_t sessId) :
			CEvent(EV_NET_CLIENT_DO_RECV, pReceiver, (PPARAM)pContainer,
					(NPARAM)0, "net_client_do_recv")
		{
			setSessId(sessId);
			if ( pContainer ) pContainer->reference();
		}

		virtual ~CEventNetClientDoRecv()
		{
			CNetContainer*	pContainer = getContainer();
			SAFE_RELEASE(pContainer);
		}

	public:
		CNetContainer* getContainer() const {
			return static_cast<CNetContainer*>(getpParam());
		}

		seqnum_t getSessionId() const {
			return static_cast<seqnum_t>(getnParam());
		}
};


class CEventNetClientRecv : public CEvent
{
	public:
		CEventNetClientRecv(result_t nresult, CNetContainer* pContainer,
							  CEventReceiver* pReceiver, seqnum_t sessId) :
			CEvent(EV_NET_CLIENT_RECV, pReceiver, (PPARAM)pContainer,
				   (NPARAM)nresult, "net_client_recv")
		{
			setSessId(sessId);
			if ( pContainer ) pContainer->reference();
		}

		virtual ~CEventNetClientRecv()
		{
			CNetContainer*	pContainer = getContainer();
			SAFE_RELEASE(pContainer);
		}

	public:
		CNetContainer* getContainer() const {
			return static_cast<CNetContainer*>(getpParam());
		}

		result_t getResult() const {
			return static_cast<result_t>(getnParam());
		}
};

/*
 * NetServerClient module statistic data
 */
typedef struct
{
	counter_t	connect;	/* Connected count */
	counter_t	send;		/* Successful send requests */
    counter_t	recv;		/* Successful receive requests */
	counter_t	fail;		/* Connect/Recv/Send fails */
} __attribute__ ((packed)) netclient_stat_t;


/*
 * CNetClient
 */
class CNetClient : public CModule, public CEventLoopThread, public CEventReceiver
{
    protected:
        CEventReceiver*     m_pParent;          /* Parent module */
        netclient_stat_t    m_stat;             /* Module statistic */

        CSocket*			m_pSocket;			/* Socket, CSocket/CSslSocket */

		mutable CMutex		m_lock;				/* Shared variable access synchonisation */
		CNetAddr			m_bindAddr;			/* Socket bind address or NETADDR_NULL */
		CString				m_strBindAddr;		/* Socket bind address or "" (for UNIX sokckets) */
		hr_time_t			m_hrConnectTimeout;	/* Connect timeout */
		hr_time_t           m_hrSendTimeout;    /* Send timeout */
		hr_time_t           m_hrRecvTimeout;    /* Recv timeout */

    public:
        CNetClient(CEventReceiver* pParent, boolean_t bSsl = false);
        virtual ~CNetClient();

    public:
        /* Statistic functions */
        virtual size_t getStatSize() const {
            return sizeof(m_stat);
        }

        virtual void getStat(void* pBuffer, size_t nSize) const {
            size_t rsize = sh_min(nSize, sizeof(m_stat));
            UNALIGNED_MEMCPY(pBuffer, &m_stat, rsize);
        }

        virtual void resetStat();
        virtual void dump(const char* strPref = "") const;

		void statConnect()		{ counter_inc(m_stat.connect); }
		void statSend()			{ counter_inc(m_stat.send); }
        void statRecv()			{ counter_inc(m_stat.recv); }
        void statFail()			{ counter_inc(m_stat.fail); }

		virtual result_t init();
		virtual void terminate();

		/*
         * Public API
         */
		virtual void connect(const CNetAddr& netAddr, seqnum_t sessId,
							 	socket_type_t sockType = SOCKET_TYPE_STREAM);
		virtual void connect(const char* strSocket, seqnum_t sessId,
							 	socket_type_t sockType = SOCKET_TYPE_STREAM);
		virtual result_t connectSync(const CNetAddr& netAddr,
								socket_type_t sockType = SOCKET_TYPE_STREAM);

		virtual void disconnect(seqnum_t nSessId = NO_SEQNUM);
		virtual result_t disconnectSync();
		virtual boolean_t isConnected() const { return m_pSocket->isOpen(); }

		virtual void send(CNetContainer* pContainer, seqnum_t sessId);
		virtual result_t sendSync(CNetContainer* pContainer);

		virtual void recv(CNetContainer* pContainer, seqnum_t sessId);
		virtual result_t recvSync(CNetContainer* pContainer);

		virtual void io(CNetContainer* pContainer, seqnum_t sessId);
		virtual result_t ioSync(CNetContainer* pContainer);

		virtual void cancel();

		void setBindAddr(const CNetAddr& bindAddr)
		{
			CAutoLock	locker(m_lock);
			m_bindAddr = bindAddr;
		}

		void setBindAddr(const char* strBindAddr)
		{
			CAutoLock	locker(m_lock);

			if ( strBindAddr ) {
				m_strBindAddr = strBindAddr;
			}
		}

		void setConnectTimeout(hr_time_t hrTimeout)
		{
			CAutoLock	locker(m_lock);
			m_hrConnectTimeout = hrTimeout;
		}

		hr_time_t getConnectTimeout() const
		{
			CAutoLock	locker(m_lock);
			return m_hrConnectTimeout;
		}

        void setTimeouts(hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout)
		{
			CAutoLock	locker(m_lock);
			m_hrSendTimeout = hrSendTimeout;
			m_hrRecvTimeout = hrRecvTimeout;
        }

		void getTimeouts(hr_time_t* phrSendTimeout, hr_time_t* phrRecvTimeout) const
		{
			CAutoLock	locker(m_lock);
			if ( phrSendTimeout ) {
				*phrSendTimeout = m_hrSendTimeout;
				if ( phrRecvTimeout ) *phrRecvTimeout = 0;
			}

			if ( phrRecvTimeout )	{
				*phrRecvTimeout = m_hrRecvTimeout;
				if ( phrSendTimeout ) *phrSendTimeout = 0;
			}
		}

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);

	private:
		void doConnect(const CNetAddr& netAddr, seqnum_t sessId, socket_type_t sockType);
		void doConnect(const char* strSocket, seqnum_t sessId, socket_type_t sockType);
		void doDisconnect(seqnum_t sessId);
		void doSend(CNetContainer* pContainer, seqnum_t sessId);
		void doRecv(CNetContainer* pContainer, seqnum_t sessId);
		void doIo(CNetContainer* pContainer, seqnum_t sessId);
};

#endif  /* __CARBON_NET_CLIENT_H_INCLUDED__ */
