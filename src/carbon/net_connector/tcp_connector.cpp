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
 *  Revision 3.0, 10.05.2017 10:48:56
 *  	Separated CNetConnector class to CTcpConnector and CUdpConnectior
 *
 */

#include <new>

#include "carbon/event.h"
#include "carbon/sync.h"
#include "carbon/net_connector/tcp_connector.h"

#define MODULE_NAME     "tcpconn"

/*******************************************************************************
 * CEventTcpConnBase
 */

CEventTcpConnBase::CEventTcpConnBase(event_type_t type, CEventReceiver* pReceiver,
                                   CNetContainer* pContainer, CSocketRef* pSocket,
                                   seqnum_t sessId) :
    CEvent()
{
	char strTmp[64] = "";

	if ( pContainer ) {
		pContainer->getDump(strTmp, sizeof(strTmp));
		pContainer->reference();
	}
	SAFE_REFERENCE(pSocket);

    init(type, pReceiver, (PPARAM)pContainer, (NPARAM)pSocket, strTmp);
	setSessId(sessId);
}

CEventTcpConnBase::~CEventTcpConnBase()
{
    CNetContainer*	pContainer = getContainer();
    CSocketRef*		pSocket = getSocket();

    SAFE_RELEASE(pContainer);
    SAFE_RELEASE(pSocket);
}

/*******************************************************************************
 * CSyncSend
 */

class CSyncSend : public CSyncBase
{
	public:
		CSyncSend(seqnum_t sessId) : CSyncBase(sessId)
		{
		}

		virtual ~CSyncSend()
		{
		}

	public:
		virtual result_t processSyncEvent();
};

result_t CSyncSend::processSyncEvent()
{
	result_t		nresult;

	if ( (CEvent*)m_pEvent == 0 )  {
		return ETIMEDOUT;
	}

	if ( m_pEvent->getType() != EV_NETCONN_SENT )  {
		log_debug(L_NETCONN, "[tcpconn] unexpected event type %d\n",  m_pEvent->getType());
		return EINVAL;
	}

	nresult = (result_t)m_pEvent->getnParam();
	return nresult;
}

/*******************************************************************************
 * CSyncRecv
 */

class CSyncRecv : public CSyncBase
{
	protected:
		dec_ptr<CNetContainer>		m_pContainer;

	public:
		CSyncRecv(seqnum_t sessId) : CSyncBase(sessId)
		{
		}

		virtual ~CSyncRecv()
		{
		}

	public:
		virtual result_t processSyncEvent();

		CNetContainer* getContainer() {
			return m_pContainer;
		}
};

result_t CSyncRecv::processSyncEvent()
{
	result_t		nresult;

	if ( (CEvent*)m_pEvent == 0 )  {
		return ETIMEDOUT;
	}

	if ( m_pEvent->getType() == EV_NETCONN_RECV )  {
		/* IO successfull */
		CEventTcpConnRecv*	pEventPacket = dynamic_cast<CEventTcpConnRecv*>((CEvent*)m_pEvent);

		shell_assert(pEventPacket);
		nresult = pEventPacket->getResult();
		if ( nresult == ESUCCESS ) {
			m_pContainer = pEventPacket->getContainer();
			shell_assert((CNetContainer*)m_pContainer);
			m_pContainer->reference();
		}
	}
	else {
		/* Wrong event */
		log_error(L_NETCONN, "[tcpconn] unexpected event type %d\n", m_pEvent->getType());
		nresult = EINVAL;
	}

	return nresult;
}


/*******************************************************************************
 * CTcpConnector Module
 */

CTcpConnector::CTcpConnector(CNetContainer* pRecvTempl, CEventReceiver* pParent,
							 size_t nMaxWorkers) :
    CModule(MODULE_NAME),
    m_pParent(pParent),
	m_pRecvTempl(pRecvTempl),
	m_nMaxWorkers(nMaxWorkers)
{
	shell_assert(pRecvTempl);
    m_pListenServer = new CTcpListenServer(this);
    m_pWorkerPool = new CTcpWorkerPool(nMaxWorkers, this);
}

CTcpConnector::~CTcpConnector()
{
	SAFE_DELETE(m_pWorkerPool);
    SAFE_DELETE(m_pListenServer);
}

/*
 * [Public API]
 *
 * Send a container through connected TCP socket
 *
 *      pContainer          container to send
 *      pSocket             connected socket
 *      pReplyReceiver      result receiver (may be NULL)
 *      sessId              unique session Id
 *
 * Return:
 *      ESUCCESS        container was successfully queued for sending
 *      EINVAL          wrong parameters (container or socket)
 *      ENOMEM          memory shortage
 *      EFAULT          all workers are busy
 */
result_t CTcpConnector::send(CNetContainer* pContainer, CSocketRef* pSocket,
                                   CEventReceiver* pReplyReceiver, seqnum_t sessId)
{
    CEventTcpConnDoSend*	pEvent;
    CTcpWorkerItem*    		pWorker;
    result_t            	nresult = ESUCCESS;

    shell_assert(pContainer);
    shell_assert(pSocket);
    shell_assert(pSocket->isOpen());

    if ( !pContainer || !pSocket || !pSocket->isOpen() ) {
        log_error(L_NETCONN, "[tcpconn] wrong parameters\n");
		statFail();
        return EINVAL;
    }

    pWorker = m_pWorkerPool->getWorker();
    if ( !pWorker )  {
        log_error(L_NETCONN, "[tcpconn] no available workers found\n");
        return EFAULT;
    }

    try {
        pEvent = new CEventTcpConnDoSend(pWorker, pContainer, pSocket,
										 pReplyReceiver, sessId);
        pWorker->sendEvent(pEvent);
    }
    catch(const std::bad_alloc& exc)  {
        log_error(L_NETCONN, "[tcpconn] failed to allocate event\n");
        nresult = ENOMEM;
		statFail();
    }

    return nresult;
}

/*
 * [Public API]
 *
 * Connect/Send/Disconnect a container through local UNIX socket
 *
 *      pContainer          container to send
 *      strSocket           socket to send
 *      pReplyReceiver      result receiver (may be NULL)
 *      sessId              unique session Id
 *
 * Return:
 *      ESUCCESS        container was successfully queued for sending
 *      EINVAL          wrong parameters (no container specified)
 *      ENOMEM          memory shortage
 *      EFAULT          all workers are busy
 */
result_t CTcpConnector::send(CNetContainer* pContainer, const char* strSocket,
                                CEventReceiver* pReplyReceiver, seqnum_t sessId)
{
	CEventTcpConnDoSendLocal*	pEvent;
	CTcpWorkerItem*    			pWorker;
    result_t            		nresult = ESUCCESS;

    shell_assert(pContainer);
    if ( !pContainer )  {
        log_error(L_NETCONN, "[tcpconn] no container is specified\n");
		statFail();
        return EINVAL;
    }

	pWorker = m_pWorkerPool->getWorker();
	if ( !pWorker )  {
		log_error(L_NETCONN, "[tcpconn] no available workers found\n");
		return EFAULT;
	}

    try {
        pEvent = new CEventTcpConnDoSendLocal(pWorker, pContainer, strSocket,
                                     pReplyReceiver, sessId);
        pWorker->sendEvent(pEvent);
    }
    catch(const std::bad_alloc& exc)  {
        log_error(L_NETCONN, "[tcpconn] failed to allocate event\n");
        nresult = ENOMEM;
		statFail();
    }

    return nresult;
}

result_t CTcpConnector::sendSync(CNetContainer* pContainer, CSocketRef* pSocket)
{
	seqnum_t		sessId = getUniqueId();
	CSyncSend		syncer(sessId);
	CEventLoop*		pEventLoop;
	result_t		nresult;

	shell_assert(m_pParent);

	pEventLoop = m_pParent->getEventLoop();
	//pEventLoop = g_pEventMap != 0 ? g_pEventMap->getEventLoop(m_pParent) : 0;
	if ( pEventLoop ) {
		pEventLoop->attachSync(&syncer);
		nresult = send(pContainer, pSocket, m_pParent, sessId);
		if ( nresult == ESUCCESS ) {
			hr_time_t hrTimeout;

			getTimeouts(&hrTimeout, 0);
			nresult = pEventLoop->waitSync(hrTimeout + HR_30SEC);
		}
		pEventLoop->detachSync();
	}
	else {
		log_error(L_GEN, "[tcpconn] event loop is not found\n");
		nresult = ESRCH;
		statFail();
	}

	return nresult;
}

result_t CTcpConnector::sendSync(CNetContainer* pContainer, const char* strSocket)
{
	seqnum_t		sessId = getUniqueId();
	CSyncSend		syncer(sessId);
	CEventLoop*		pEventLoop;
	result_t		nresult;

	shell_assert(m_pParent);

	pEventLoop = m_pParent->getEventLoop();
	//pEventLoop = g_pEventMap != 0 ? g_pEventMap->getEventLoop(m_pParent) : 0;
	if ( pEventLoop ) {
		pEventLoop->attachSync(&syncer);
		nresult = send(pContainer, strSocket, m_pParent, sessId);
		if ( nresult == ESUCCESS ) {
			hr_time_t hrTimeout;

			getTimeouts(&hrTimeout, 0);
			nresult = pEventLoop->waitSync(hrTimeout + HR_30SEC);
		}
		pEventLoop->detachSync();
	}
	else {
		log_error(L_GEN, "[tcpconn] event loop is not found\n");
		nresult = ESRCH;
		statFail();
	}

	return nresult;
}

/*
 * [Public API]
 *
 * Connect to the remote host, send a container, receive a reply container,
 * disconnect from the remote host and send reply (EV_PACKET) containing
 * a received container.
 *
 *      pContainer          container to send
 *      destAddr            remote host address
 *      pReplyReceiver      result receiver (may NOT be NULL)
 *      sessId              unique session Id
 *
 * Return:
 *      ESUCCESS        container was successfully queued for IO
 *      EINVAL          wrong parameters (no container specified)
 *      ENOMEM          memory shortage
 *      EFAULT          all workers are busy
 */
result_t CTcpConnector::io(CNetContainer* pContainer, const CNetAddr& destAddr,
                                 CEventReceiver* pReplyReceiver, seqnum_t sessId)
{
	CEventTcpConnDoIo*	pEvent;
    CTcpWorkerItem*		pWorker;
    result_t            nresult = ESUCCESS;

    shell_assert(pContainer);
    shell_assert(destAddr != NETADDR_NULL);
    shell_assert(pReplyReceiver);

    if ( !pContainer )  {
        log_error(L_NETCONN, "[tcpconn] invalid info-container\n");
		statFail();
        return EINVAL;
    }

    pWorker = m_pWorkerPool->getWorker();
    if ( !pWorker )  {
        log_error(L_NETCONN, "[tcpconn] no available workers found\n");
        return EFAULT;
    }

    try {
        pEvent = new CEventTcpConnDoIo(pWorker, pContainer, destAddr,
										   pReplyReceiver, sessId);
        pWorker->sendEvent(pEvent);
    }
    catch(const std::bad_alloc& exc)  {
        log_error(L_NETCONN, "[tcpconn] failed to allocate event\n");
        nresult = ENOMEM;
		statFail();
    }

    return nresult;
}

result_t CTcpConnector::ioSync(CNetContainer* pContainer, const CNetAddr& destAddr,
							   CNetContainer** ppOutContainer)
{
	seqnum_t		sessId = getUniqueId();
	CSyncRecv			syncer(sessId);
	CEventLoop*		pEventLoop;
	result_t		nresult;

	pEventLoop = m_pParent->getEventLoop();
	//pEventLoop = g_pEventMap != 0 ? g_pEventMap->getEventLoop(m_pParent) : 0;
	if ( pEventLoop ) {
		pEventLoop->attachSync(&syncer);
		nresult = io(pContainer, destAddr, m_pParent, sessId);
		if ( nresult == ESUCCESS ) {
			hr_time_t hrSendTimeout, hrRecvTimeout;

			getTimeouts(&hrSendTimeout, &hrRecvTimeout);
			nresult = pEventLoop->waitSync(hrSendTimeout + hrRecvTimeout +
											   getConnectTimeout() + HR_30SEC);
			if ( nresult == ESUCCESS )  {
				*ppOutContainer = syncer.getContainer();
				(*ppOutContainer)->reference();
			}
		}
		pEventLoop->detachSync();
	}
	else {
		log_error(L_GEN, "[tcpconn] event loop is not found\n");
		nresult = ESRCH;
		statFail();
	}

	return nresult;
}


void CTcpConnector::resetStat()
{
    counter_reset_struct(m_stat);
}


/*
 * [Public API]
 *
 * Start accepting an incoming connections
 *
 * Return:
 *      ESUCCESS, ...
 */
result_t CTcpConnector::startListen(const CNetAddr& listenAddr)
{
    result_t    nresult;

	m_pListenServer->setAddr(listenAddr);
    nresult = m_pListenServer->start();

    return nresult;
}

result_t CTcpConnector::startListen(const char* strSocket)
{
	result_t    nresult;

	m_pListenServer->setAddr(strSocket);
	nresult = m_pListenServer->start();

	return nresult;
}

/*
 * [Public API]
 *
 * Stop accepting an incoming connections
 */
void CTcpConnector::stopListen()
{
    m_pListenServer->stop();
}

/*
 * [Initialisation API]
 *
 * Prepare to perform send/receive network operation
 *
 * Return:
 *      ESUCCESS, ...
 *
 * Note: The listening server is not started automatically.
 */
result_t CTcpConnector::init()
{
    result_t    nresult;

    nresult = CModule::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    resetStat();

    nresult = m_pWorkerPool->start();
    if ( nresult != ESUCCESS )  {
        log_error(L_NETCONN, "[netconn] failed to start worker pool, result: %d\n", nresult);
    }

    return nresult;
}

/*
 * [Initialisation API]
 *
 * Cancel all I/O operations, stop workers and listener server
 */
void CTcpConnector::terminate()
{
	stopListen();
	m_pWorkerPool->stop();

	if ( (CNetContainer*)m_pRecvTempl ) {
		shell_assert_ex(m_pRecvTempl->getRefCount() == 1,
						"refcount = %d\n", m_pRecvTempl->getRefCount());
	}

	CModule::terminate();
}

/*******************************************************************************
 * Debugging support
 */

#if CARBON_DEBUG_DUMP

void CTcpConnector::dump(const char* strPref) const
{
    log_dump("%sNetConnector statistic:\n", strPref);
    log_dump("     incoming connections:    %d\n", counter_get(m_stat.client));
    log_dump("     incoming errors:         %d\n", counter_get(m_stat.client_fail));
    log_dump("     received containers:     %d\n", counter_get(m_stat.recv));
    log_dump("     sent containers:         %d\n", counter_get(m_stat.send));
    log_dump("     I/O errors:              %d\n", counter_get(m_stat.fail));
    log_dump("     pool workers:            %d\n", counter_get(m_stat.worker));
	log_dump("     pool worker errors:      %d\n", counter_get(m_stat.worker_fail));
}

#endif /* CARBON_DEBUG_DUMP */
