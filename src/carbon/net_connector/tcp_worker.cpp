/*
 *  Carbon framework
 *  Tcp connector Worker thread pool
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.06.2015 18:23:21
 *      Initial revision.
 */

#include <new>

#include "carbon/event.h"
#include "carbon/packet_io.h"
#include "carbon/event/remote_event.h" /* TODO, FIX */
#include "carbon/net_connector/tcp_listen.h"
#include "carbon/net_connector/tcp_connector.h"
#include "carbon/net_connector/tcp_worker.h"

/*
 * Default I/O operation timeouts
 */
#define TCPCONN_SEND_TIMEOUT          	HR_4SEC
#define TCPCONN_RECV_TIMEOUT          	HR_10SEC
#define TCPCONN_CONNECT_TIMEOUT			HR_10SEC


/*******************************************************************************
 * CTcpWorkerItem class
 */

CTcpWorkerItem::CTcpWorkerItem(CThreadPool* pParent, const char* strName) :
    CThreadPoolItem(pParent, strName)
{
}

CTcpWorkerItem::~CTcpWorkerItem()
{
}

CTcpWorkerPool* CTcpWorkerItem::getParent()
{
    shell_assert(m_pParent);
    return dynamic_cast<CTcpWorkerPool*>(m_pParent);
}

/*
 * Send a reply packet
 *
 *      pReplyReceiver      reply packet destination
 *      nresult				I/O result code
 *      pContainer          reply packet container
 *      pSocket             socket (optional, may be 0)
 *      sessId              I/O session unique ID
 */
void CTcpWorkerItem::notifyRecv(CEventReceiver* pReplyReceiver, result_t nresult,
                    		CNetContainer* pContainer, CSocketRef* pSocket, seqnum_t sessId)
{
	CEventTcpConnRecv*	pEvent;

	pEvent = new CEventTcpConnRecv(pReplyReceiver, nresult, pContainer, pSocket, sessId);
    appSendEvent(pEvent);
}

/*
 * Send reply result packet
 *
 *      pReplyReceiver      reply packet destination
 *      nresult             result code
 *      sessId              session unique ID
 */
void CTcpWorkerItem::notifySend(CEventReceiver* pReplyReceiver,
                                    result_t nresult, seqnum_t sessId)
{
    CEvent*     pEvent;
    char        stmp[32];

    _tsnprintf(stmp, sizeof(stmp), "sess=%u, nresult=%d", sessId, nresult);
    pEvent = new CEvent(EV_NETCONN_SENT, pReplyReceiver, NULL, (NPARAM)nresult, stmp);
	pEvent->setSessId(sessId);
    appSendEvent(pEvent);
}

/*
 * Receive a data from socket with default timeout
 *
 *      pSocket             open socket
 *
 * Note: Event EV_NETCONN_RECEIVE is received from listen server:
 *          - receive new packet from the net;
 *          - on success: send EV_NETCONN_RECV to the default receiver with no session ID;
 *          - on failed: do nothing
 */
void CTcpWorkerItem::processReceive(CSocketRef* pSocket)
{
    CTcpWorkerPool*			pParent = getParent();
    hr_time_t               hrRecvTimeout;
    dec_ptr<CNetContainer>  pContainer, pContainerOriginal;
    result_t                nresult;

	pContainerOriginal = pParent->getParent()->getRecvTemplRef();
	pContainer = pContainerOriginal->clone();

	log_trace(L_NETCONN, "[tcpconn_work] receiving a container...\n");
    pParent->getTimeouts(0, &hrRecvTimeout);

    try {
        nresult = pContainer->receive(*pSocket, hrRecvTimeout);
		if ( nresult == ESUCCESS )  {
			log_trace(L_NETCONN, "[tcpconn_work] received a container\n");
		}
        else {
            log_debug(L_NETCONN, "[tcpconn_work] failed to receive container, result: %d\n", nresult);
        }
    }
    catch(const std::bad_alloc& exc)  {
        log_error(L_NETCONN, "[tcpconn_work] out of memory\n");
        nresult = ENOMEM;
    }

    if ( nresult == ESUCCESS )  {
		pParent->statRecv();

        if ( logger_is_enabled(LT_DEBUG|L_NETCONN_IO) )  {
            char    strTmp[128];

			pContainer->getDump(strTmp, sizeof(strTmp));
            log_debug(L_NETCONN_IO, "[tcpconn_work] >>> Recv container: %s\n", strTmp);
        }

		/*
		 * TODO, FIX
		 * Special processing of the CRemoteEventContainer container
		 */
		CRemoteEventContainer*	pRemoteContainer;
		pRemoteContainer = dynamic_cast<CRemoteEventContainer*>((CNetContainer*)pContainer);
		if ( pRemoteContainer != 0 )  {
			dec_ptr<CRemoteEvent>	pRemoteEvent = pRemoteContainer->getEvent();
			CEventReceiver*			pReceiver = pRemoteEvent->getReceiver();

			pRemoteEvent->reference();
			if ( pReceiver == 0 )  {
				pRemoteEvent->setReceiver(getParent()->getReceiver());
			}
			appSendEvent(pRemoteEvent);
		}
		else {
			notifyRecv(getParent()->getReceiver(), ESUCCESS, pContainer, pSocket, NO_SEQNUM);
		}
    }
    else {
        pParent->statFail();
    }
}

/*
 * Send a container to the connected socket EV_NETCONN_PACKET_SEND)
 *
 *      pContainer      	container to send
 *      pSocket         	connected socket
 *      pReplyReceiver      reply packet receiver
 *      sessId              unique session Id
 *
 * Note: reply container is sent if a pReplyReceiver is not NULL.
 */
void CTcpWorkerItem::processSend(CNetContainer* pContainer, CSocketRef* pSocket,
                                        CEventReceiver* pReplyReceiver, seqnum_t sessId)
{
    CTcpWorkerPool*		pParent = getParent();
    hr_time_t       	hrSendTimeout;
    result_t        	nresult;

    pParent->getTimeouts(&hrSendTimeout, 0);

	log_trace(L_NETCONN, "[tcpconn_work(%d)] sending a container\n", sessId);
    nresult = pContainer->send(*pSocket, hrSendTimeout);
    if ( nresult == ESUCCESS ) {
		pParent->statSend();

        if ( logger_is_enabled(LT_DEBUG|L_NETCONN_IO) )  {
            char    strTmp[128];

            pContainer->getDump(strTmp, sizeof(strTmp));
            log_debug(L_NETCONN_IO, "[tcpconn_work] <<< Sent container: %s\n", strTmp);
        }
		else {
			log_trace(L_NETCONN, "[tcpconn_work(%d)] container send\n", sessId);
		}
    }
    else {
        log_debug(L_NETCONN, "[tcpconn_work(%d)] failed to send container, result: %d\n",
				  sessId, nresult);
        pParent->statFail();
    }

    if ( pReplyReceiver ) {
        notifySend(pReplyReceiver, nresult, sessId);
    }
}

/*
 * Send a container to the UNIX Domain socket (EV_NETCONN_SEND_LOCAL)
 *
 *      pContainer      	container to send
 *      strSocket         	local UNIX socket
 *      pReplyReceiver      reply packet receiver
 *      sessId              unique session Id
 *
 * Note: reply container is sent if a pReplyReceiver is not NULL.
 */
void CTcpWorkerItem::processSendLocal(CNetContainer* pContainer, const char* strSocket,
							  CEventReceiver* pReplyReceiver, seqnum_t sessId)
{
	dec_ptr<CSocketRef>	pSocket = new CSocketRef;
	CTcpWorkerPool*    	pParent = getParent();
	hr_time_t			hrSendTimeout;
	result_t        	nresult;

	pParent->getTimeouts(&hrSendTimeout, 0);

	log_trace(L_NETCONN, "[tcpconn_work(%d)] sending a container to '%s'\n",
			  sessId, strSocket);

	nresult = pSocket->connect(strSocket, pParent->getConnectTimeout(), SOCKET_TYPE_STREAM);
	if ( nresult == ESUCCESS )  {
		nresult = pContainer->send(*pSocket, hrSendTimeout);
		if ( nresult == ESUCCESS ) {
			pParent->statSend();

			if ( logger_is_enabled(LT_DEBUG|L_NETCONN_IO) )  {
				char    strTmp[128];

				pContainer->getDump(strTmp, sizeof(strTmp));
				log_debug(L_NETCONN_IO, "[tcpconn_work] <<< Sent container: %s\n", strTmp);
			}
			else {
				log_trace(L_NETCONN, "[tcpconn_work(%d)] container send\n", sessId);
			}
		}
	}
	else {
		log_trace(L_NETCONN, "[tcpconn_work(%d)] connection failure, result %d\n", sessId, nresult);
		pParent->statFail();
	}

	pSocket->close();

	if ( pReplyReceiver ) {
		notifySend(pReplyReceiver, nresult, sessId);
	}
}

/*
 * Process packet I/O
 *
 *      pContainer          container to send to the remote host
 *      destAddr            remote host address
 *      pReplyReceiver      reply packet receiver
 *      sessId              unique session Id
 */
void CTcpWorkerItem::processIo(CNetContainer* pContainer, const CNetAddr& destAddr,
                                      CEventReceiver* pReplyReceiver, seqnum_t sessId)
{
    CTcpWorkerPool*		pParent = getParent();
    const CNetAddr      bindAddr(pParent->getBindAddr());
    hr_time_t           hrSendTimeout, hrRecvTimeout;
    result_t            nresult;

	shell_assert(pContainer);
    pParent->getTimeouts(&hrSendTimeout, &hrRecvTimeout);
    CPacketIo	IO(hrSendTimeout+hrRecvTimeout+pParent->getConnectTimeout());

	log_trace(L_NETCONN, "[tcpconn_work(%d)] executing I/O...\n", sessId);

    try {
        dec_ptr<CNetContainer>  outContainerPtr = pContainer->clone();

        nresult = IO.execute(pContainer, outContainerPtr, destAddr, bindAddr);
        if ( nresult == ESUCCESS )  {
            if ( logger_is_enabled(LT_DEBUG|L_NETCONN_IO) )  {
                char    strTmpOut[128], strTmpIn[128];

                pContainer->getDump(strTmpOut, sizeof(strTmpOut));
                outContainerPtr->getDump(strTmpIn, sizeof(strTmpIn));
                log_debug(L_NETCONN_IO, "[tcpconn_work] <<< I/O Sent container: %s\n", strTmpOut);
                log_debug(L_NETCONN_IO, "[tcpconn_work] >>> I/O Recv container: %s\n", strTmpIn);
            }
			else {
				log_trace(L_NETCONN, "[tcpconn_work(%d)] I/O executed success\n", sessId);
			}

			pParent->statRecv();
			pParent->statSend();

            notifyRecv(pReplyReceiver, ESUCCESS, outContainerPtr, 0, sessId);
        }
        else {
            log_debug(L_NETCONN, "[tcpconn_work(%d)] failed packet I/O to %s, result: %d\n",
                      sessId, destAddr.cs(), nresult);
        }
    }
    catch(const std::bad_alloc& exc)  {
        log_error(L_NETCONN, "[tcpconn_work(%d)] out of memory\n", sessId);
        nresult = ENOMEM;
    }

    if ( nresult != ESUCCESS )  {
        notifyRecv(pReplyReceiver, nresult, 0, 0, sessId);
        pParent->statFail();
    }
}

/*
 * Event processor
 *
 *      pEvent      event object to process
 *
 * Return:
 *      TRUE        event processed
 *      FALSE       event is not processed
 */
boolean_t CTcpWorkerItem::processEvent(CEvent* pEvent)
{
    CEventTcpConnDoConnect* 	pEventConnect;
	CEventTcpConnDoSend*		pEventSend;
	CEventTcpConnDoSendLocal*	pEventSendLocal;
	CEventTcpConnDoIo*	    	pEventIo;
    boolean_t               	bProcessed = FALSE;

    switch ( pEvent->getType() )  {
        case EV_NETCONN_DO_CONNECT:
			pEventConnect = dynamic_cast<CEventTcpConnDoConnect*>(pEvent);
            shell_assert(pEventConnect);
            if ( pEventConnect ) {
                processReceive(pEventConnect->getSocket());
            }
            bProcessed = TRUE;
            break;

        case EV_NETCONN_DO_SEND:
            pEventSend = dynamic_cast<CEventTcpConnDoSend*>(pEvent);
            shell_assert(pEventSend);
            if ( pEventSend )  {
                processSend(pEventSend->getContainer(), pEventSend->getSocket(),
                       		pEventSend->getReplyReceiver(), pEventSend->getSessId());
            }
            bProcessed = TRUE;
            break;

		case EV_NETCONN_DO_SEND_LOCAL:
			pEventSendLocal = dynamic_cast<CEventTcpConnDoSendLocal*>(pEvent);
			shell_assert(pEventSendLocal);
			if ( pEventSendLocal )  {
				processSendLocal(pEventSendLocal->getContainer(), pEventSendLocal->getLocalSocket(),
								 pEventSendLocal->getReplyReceiver(), pEventSendLocal->getSessId());
			}
			bProcessed = TRUE;
			break;

        case EV_NETCONN_DO_IO:
            pEventIo = dynamic_cast<CEventTcpConnDoIo*>(pEvent);
            shell_assert(pEventIo);
            if ( pEventIo )  {
                processIo(pEventIo->getContainer(), pEventIo->getDestAddr(),
						  pEventIo->getReplyReceiver(), pEventIo->getSessId());
            }
            bProcessed = TRUE;
            break;
    }

    return bProcessed;
}


/*******************************************************************************
 * CTcpWorkerPool class
 */

CTcpWorkerPool::CTcpWorkerPool(size_t nMaxWorkers, CTcpConnector* pParent) :
    CThreadPool(nMaxWorkers, "TcpConnWorkerPool"),
    m_pParent(pParent),
    m_bindAddr(NETADDR_NULL),
    m_hrSendTimeout(TCPCONN_SEND_TIMEOUT),
    m_hrRecvTimeout(TCPCONN_RECV_TIMEOUT),
	m_hrConnectTimeout(TCPCONN_CONNECT_TIMEOUT)
{
    shell_assert(pParent);
}

CTcpWorkerPool::~CTcpWorkerPool()
{
}

CTcpWorkerItem* CTcpWorkerPool::getWorker()
{
    CThreadPoolItem*    pThread;

    pThread = CThreadPool::get();
    if ( pThread )  {
		m_pParent->statWorkerCount(getThreadCount());
        return (CTcpWorkerItem*)pThread;
    }

	m_pParent->statWorkerFail();
    return 0;
}

CEventReceiver* CTcpWorkerPool::getReceiver() const
{
    return m_pParent->getReceiver();
}


/*
 * Create pool instance
 *
 *      strName         instance name
 *
 * Return: new instance object
 */
CThreadPoolItem* CTcpWorkerPool::createThread(const char* strName)
{
    return new CTcpWorkerItem(this, strName);
}

void CTcpWorkerPool::statRecv()		{ m_pParent->statRecv(); }
void CTcpWorkerPool::statSend()		{ m_pParent->statSend(); }
void CTcpWorkerPool::statFail()		{ m_pParent->statFail(); }

/*
 * Start worker pool
 */
result_t CTcpWorkerPool::start()
{
    result_t    nresult;

    nresult = CThreadPool::init();
    m_pParent->statWorkerCount(getThreadCount());
    return nresult;
}

void CTcpWorkerPool::stop()
{
	log_trace(L_NETCONN, "[tcpconn_work] terminating worker pool\n");
    CThreadPool::terminate();
	log_trace(L_NETCONN, "[tcpconn_work] worker pool has been terminated\n");
}
