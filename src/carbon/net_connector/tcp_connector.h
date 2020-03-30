/*
 *  Carbon framework
 *  TCP connector module
 *
 *  Copyright (c) 2015-2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 10.06.2015 22:22:05
 *      Initial revision.
 *
 *  Revision 2.0, 20.07.2016 15:17:01
 *      Modified API functions
 *
 *  Revision 2.1, 31.08.2016 14:05:03
 *      Modified API function names
 *
 *  Revision 3.0, 10.05.2017 10:48:56
 *  	Separated CNetConnector class to CTcpConnector and CUdpConnectior
 */
/*
 * Purpose:
 *      Synchronous/Asynchronous network TCP I/O operations
 *
 * Constructor:
 *      CTcpConnector(CNetContainer* pRecvTempl, EventReceiver* pParent, size_t nMaxWorkers = 16);
 *          pRecvTempl      sample container for receiving the new containers
 *      	pParent         default container receiver (may be 0)
 *          nMaxWorkers     maximum sumalteniously running workers
 *
 *
 * I) Initialisation/Deinitialisation API:
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *      result_t init();
 *          Prepare to perform network I/O operations (listen server is not started);
 *
 *      result_t startListen(const CNetAddr& listenAddr);
 *      result_t startListen(const char* strSocket);
 *          Start accepting incoming connections.
 *          listenAddr      address to listen
 *          strSocket       UNIX local sioscket to listen
 *
 *      void stopListen();
 *          Cancel accepting incoming connections.
 *
 *      void terminate();
 *          Cancel all I/O operations and stop all workers (including listen server).
 *
 *
 * II) Settings API (to be called before init()):
 *     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *      void setBindAddr(const CNetAddr&);
 *          Setup source network address for send().
 *
 *      const CNetAddr& getBindAddr() const;
 *          Obtain current source network address.
 *
 *      void setTimeouts(hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout);
 *          Setup send/recv timeouts for I/O operations.
 *
 *      void setConnectTimeout(hr_time_t hrTimeout);
 *          Setup connect timeout.
 *
 *      void getTimeouts(hr_time_t* phrSendTimeout, hr_time_t* phrRecvTimeout);
 *          Obtain current send/recv timeouts.
 *
 *      hr_time_t getConnectTimeout() const;
 *      	Obtain current connect to remote host timeout.
 *
 *
 * III) Public API:
 *      ~~~~~~~~~~~
 *
 *      result_t send(CNetContainer* pContainer, CSocketRef* pSocket,
 *                 		CEventReceiver* pReplyReceiver = 0, seqnum_t sessId = NO_SEQNUM);
 *          Send a container through a connected socket.
 *          A reply packet is send if a pReplyReceiver is not NULL.
 *          Optional event: CEvent, EV_NETCONN_SENT
 *
 *      result_t send(CNetContainer* pContainer, const char* strSocket,
 *               		CEventReceiver* pReplyReceiver = 0, seqnum_t sessId = NO_SEQNUM);
 *          Connect/Send/Disconnect a container through a local UNIX socket.
 *          A reply packet is send if a pReplyReceiver is not NULL.
 *          Optional event: CEvent, EV_NETCONN_SENT (NPARAM = nresult)
 *
 * 		result_t sendSync(CNetContainer* pContainer, CSocketRef* pSocket);
 *          Send a container through a connected socket in syncronous mode.
 *
 * 		result_t sendSync(CNetContainer* pContainer, const char* strSocket);
 *          Send a container through a socket in syncronous mode.
 *
 *      result_t io(CNetContainer* pContainer, const CNetAddr& destAddr,
 *             			CEventReceiver* pReplyReceiver, seqnum_t sessId);
 *          Connect to the remote host, send a container, receive a reply container,
 *          disconnect from the remote host and send reply containing
 *          a received container.
 *
 *          Result event: CEventNetConnRecv, EV_NETCONN_RECV
 *
 *      result_t ioSync(CNetContainer* pContainer, const CNetAddr& destAddr,
 *							CNetContainer** ppOutContainer);
 *
 */

#ifndef __CARBON_TCP_CONNECTOR_H_INCLUDED__
#define __CARBON_TCP_CONNECTOR_H_INCLUDED__

#include "shell/config.h"
#include "shell/netaddr.h"
#include "shell/counter.h"
#include "shell/dec_ptr.h"

#include "carbon/carbon.h"
#include "carbon/module.h"
#include "carbon/event.h"
#include "carbon/net_container.h"
#include "carbon/net_connector/tcp_listen.h"
#include "carbon/net_connector/tcp_worker.h"

/*
 * TCP Connector events base class
 *
 * 		type			event type, EV_xxx
 * 		pReceiver		reply receiver (may be NULL)
 * 		pContainer		net container (may be NULL)
 * 		pSocket			open tcp socket (may be NULL)
 * 		sessId			I/O session id (may ne NO_SEQNUM)
 *
 * 		PPARAM		pContainer
 * 		NPARAM		pSocket
 */

class CEventTcpConnBase : public CEvent
{
    public:
        CEventTcpConnBase(event_type_t type, CEventReceiver* pReceiver,
                         CNetContainer* pContainer, CSocketRef* pSocket,
                         seqnum_t sessId);
    protected:
        virtual ~CEventTcpConnBase();

    public:
        virtual CNetContainer* getContainer() const {
            return static_cast<CNetContainer*>(getpParam());
        }

        virtual CSocketRef* getSocket() const {
            return reinterpret_cast<CSocketRef*>(getnParam());
        }
};

class CEventTcpConnRecv : public CEventTcpConnBase
{
	private:
		result_t	m_nresult;

	public:
		CEventTcpConnRecv(CEventReceiver* pReceiver, result_t nresult,
					CNetContainer* pContainer, CSocketRef* pSocket, seqnum_t sessId) :
			CEventTcpConnBase(EV_NETCONN_RECV, pReceiver, pContainer, pSocket, sessId),
			m_nresult(nresult)
		{
		}

		virtual ~CEventTcpConnRecv()
		{
		}

	public:
		result_t getResult() const {
			return m_nresult;
		}
};

class CEventTcpConnDoSend : public CEventTcpConnBase
{
	private:
		CEventReceiver*		m_pReplyReceiver;

	public:
		CEventTcpConnDoSend(CEventReceiver* pReceiver, CNetContainer* pContainer,
						CSocketRef* pSocket, CEventReceiver* pReplyReceiver, seqnum_t sessId) :
			CEventTcpConnBase(EV_NETCONN_DO_SEND, pReceiver, pContainer, pSocket, sessId),
			m_pReplyReceiver(pReplyReceiver)
		{
		}

		virtual ~CEventTcpConnDoSend()
		{
		}

	public:
		CEventReceiver* getReplyReceiver() const {
			return m_pReplyReceiver;
		}
};

class CEventTcpConnDoSendLocal : public CEventTcpConnBase
{
	private:
		CEventReceiver*		m_pReplyReceiver;
		CString             m_strSocket;

	public:
		CEventTcpConnDoSendLocal(CEventReceiver* pReceiver, CNetContainer* pContainer,
							 const char* strSocket, CEventReceiver* pReplyReceiver, seqnum_t sessId) :
			CEventTcpConnBase(EV_NETCONN_DO_SEND_LOCAL, pReceiver, pContainer, 0, sessId),
			m_pReplyReceiver(pReplyReceiver),
			m_strSocket(strSocket)
		{
		}

		virtual ~CEventTcpConnDoSendLocal()
		{
		}

	public:
		CEventReceiver* getReplyReceiver() const {
			return m_pReplyReceiver;
		}

		const char* getLocalSocket() const {
			return m_strSocket;
		}
};

class CEventTcpConnDoIo : public CEventTcpConnBase
{
	private:
		CNetAddr			m_destAddr;
		CEventReceiver*		m_pReplyReceiver;

	public:
		CEventTcpConnDoIo(CEventReceiver* pReceiver, CNetContainer* pContainer,
					  const CNetAddr& destAddr, CEventReceiver* pReplyReceiver, seqnum_t sessId) :
			CEventTcpConnBase(EV_NETCONN_DO_IO, pReceiver, pContainer, 0, sessId),
			m_destAddr(destAddr),
			m_pReplyReceiver(pReplyReceiver)
		{
		}
		virtual ~CEventTcpConnDoIo()
		{
		}

	public:
		const CNetAddr& getDestAddr() const {
			return m_destAddr;
		}

		CEventReceiver* getReplyReceiver() const {
			return m_pReplyReceiver;
		}
};


/*
 * Tcp Connector module statistic data
 */
typedef struct
{
    counter_t   client;   				/* Accepted clients */
    counter_t   client_fail;			/* Accepted client internal errors */

    counter_t   recv;           		/* Receive requests */
    counter_t   send;           		/* Send requests */
    counter_t   fail;        			/* I/O fails */

    counter_t   worker;					/* Current worker pool thread count */
	counter_t	worker_fail;			/* Worker request fails */
} __attribute__ ((packed)) tcpconn_stat_t;


#define TCP_CONN_WORKER_MAX			16

/*
 * CTcpConnector
 */
class CTcpConnector : public CModule
{
    protected:
        CEventReceiver*         m_pParent;          /* Received container event receiver */
        CTcpListenServer*      	m_pListenServer;    /* TCP Listening server */
        CTcpWorkerPool*       	m_pWorkerPool;      /* I/O Worker threads */
        dec_ptr<CNetContainer>	m_pRecvTempl;	    /* Receive container template */
        size_t                  m_nMaxWorkers;		/* Maximum sumalteniously running workers */

		tcpconn_stat_t          m_stat;             /* Module statistic */

    public:
        CTcpConnector(CNetContainer* pRecvTempl, CEventReceiver* pParent, size_t nMaxWorkers = TCP_CONN_WORKER_MAX);
        virtual ~CTcpConnector();

    public:
        CEventReceiver* getReceiver() const {
            return m_pParent;
        }

        CTcpWorkerItem* getWorker() {
            shell_assert(m_pWorkerPool);
            return m_pWorkerPool->getWorker();
        }

        CNetContainer* getRecvTemplRef() {
            SAFE_REFERENCE((CNetContainer*)m_pRecvTempl);
            return m_pRecvTempl;
        }

        /* Statistic functions */
        virtual size_t getStatSize() const {
            return sizeof(m_stat);
        }

        virtual void getStat(void* pBuffer, size_t nSize) const {
            size_t rsize = sh_min(nSize, sizeof(m_stat));
            UNALIGNED_MEMCPY(pBuffer, &m_stat, rsize);
        }

        virtual void resetStat();

        void statClient()		{ counter_inc(m_stat.client); }
        void statClientFail()	{ counter_inc(m_stat.client_fail); }
        void statRecv()			{ counter_inc(m_stat.recv); }
        void statSend()			{ counter_inc(m_stat.send); }
        void statFail()			{ counter_inc(m_stat.fail); }
        void statWorkerCount(size_t count)  { counter_set(m_stat.worker, count); }
		void statWorkerFail()	{ counter_inc(m_stat.worker_fail); }


        /*
         * Public API
         */
		boolean_t isTerminating() const {
			return m_pWorkerPool ? m_pWorkerPool->isTerinating() : TRUE;
		}

        result_t startListen(const CNetAddr& listenAddr);
        result_t startListen(const char* strSocket);
        void stopListen();

        void setBindAddr(const CNetAddr& bindAddr) {
            m_pWorkerPool->setBindAddr(bindAddr);
        }

        const CNetAddr& getBindAddr() const {
            return m_pWorkerPool->getBindAddr();
        }

        void setTimeouts(hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout)  {
            m_pWorkerPool->setTimeouts(hrSendTimeout, hrRecvTimeout);
        }

		void setConnectTimeout(hr_time_t hrTimeout)  {
			m_pWorkerPool->setConnectTimeout(hrTimeout);
		}

        void getTimeouts(hr_time_t* phrSendTimeout, hr_time_t* phrRecvTimeout) const {
            m_pWorkerPool->getTimeouts(phrSendTimeout, phrRecvTimeout);
        }

		hr_time_t getConnectTimeout() const {
			return m_pWorkerPool->getConnectTimeout();
		}

        virtual result_t send(CNetContainer* pContainer, CSocketRef* pSocket,
                            CEventReceiver* pReplyReceiver = 0, seqnum_t sessId = NO_SEQNUM);

        virtual result_t send(CNetContainer* pContainer, const char* strSocket,
                            CEventReceiver* pReplyReceiver = 0, seqnum_t sessId = NO_SEQNUM);

		virtual result_t sendSync(CNetContainer* pContainer, CSocketRef* pSocket);
		virtual result_t sendSync(CNetContainer* pContainer, const char* strSocket);

		virtual result_t io(CNetContainer* pContainer, const CNetAddr& destAddr,
							CEventReceiver* pReplyReceiver, seqnum_t sessId);

		virtual result_t ioSync(CNetContainer* pContainer, const CNetAddr& destAddr,
							CNetContainer** ppOutContainer);

		virtual result_t init();
		virtual void terminate();

#if CARBON_DEBUG_DUMP
	public:
        virtual void dump(const char* strPref = "") const;
#endif /* CARBON_DEBUG_DUMP */
};

#endif  /* __CARBON_TCP_CONNECTOR_H_INCLUDED__ */
