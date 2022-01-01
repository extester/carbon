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
 *  Revision 1.0, 23.06.2016 17:24:25
 *      Initial revision.
 */

#include <new>

#include "carbon/carbon.h"
#include "carbon/net_server/net_server.h"
#include "carbon/net_server/net_server_connection.h"

#define MODULE_NAME         "net_server_conn"

/*******************************************************************************
 * CNetServerConnection object
 */

CNetServerConnection::CNetServerConnection(CSocketRef* pSocket, CNetServer* pParent) :
    CEventLoopThread(MODULE_NAME),
    CEventReceiver(this, MODULE_NAME),
    m_pSocket(pSocket),
    m_pParent(pParent),
	m_bRecvBreak(FALSE),
	m_bFailSent(FALSE)
{
    shell_assert(m_pSocket->isOpen());
    m_pSocket->breakerEnable();
    m_pSocket->reference();
}

CNetServerConnection::~CNetServerConnection()
{
    //shell_assert(!CEventLoopThread::isRunning());
}

CNetContainer* CNetServerConnection::getRecvTemplRef()
{
	return m_pParent->getRecvTemplRef();
}

void CNetServerConnection::notifySend(result_t nOpResult, seqnum_t nSessId)
{
	CEvent*     pEvent;

	if ( nSessId != NO_SEQNUM ) {
		pEvent = new CEvent(EV_NET_SERVER_SENT, m_pParent->getReceiver(),
							(PPARAM)0, (NPARAM)nOpResult, "net_serv_sent");
		pEvent->setSessId(nSessId);
		appSendEvent(pEvent);
	}
}

/*
 * Send a container with predefined timeout
 *
 *      pContainer		container to send
 *      nSessId			unique session Id (NO_SEQNUM - do not send reply)
 *
 * Result: send EV_NET_SERVER_SENT notification
 */
void CNetServerConnection::doSend(CNetContainer* pContainer, seqnum_t nSessId)
{
    result_t    nresult;

    if ( m_pSocket->isOpen() )  {
		log_trace(L_NETSERV, "[netserv_con(%d)] sending a container\n", nSessId);

        nresult = pContainer->send(*m_pSocket, m_pParent->getSendTimeout());

		if ( nresult == ESUCCESS || nresult == ECANCELED )  {
			m_bFailSent = FALSE;
			if ( nresult == ESUCCESS ) {
				if ( logger_is_enabled(LT_TRACE|L_NETSERV_IO) ) {
					char    strTmp[128];

					pContainer->getDump(strTmp, sizeof(strTmp));
					log_trace(L_NETCONN_IO, "[netserv_con] <<< Sent container: %s\n", strTmp);
				}
				else {
					log_trace(L_NETSERV, "[netserv_con(%d)] container sent\n", nSessId);
				}
				m_pParent->statSend();
			}
		}
		else {
            log_error(L_NETSERV, "[netserv_con(%d)] failed to send container, result %d\n",
					  nSessId, nresult);
			m_pParent->statFail();
        }

		notifySend(nresult, nSessId);

        if ( IS_NETWORK_CONNECTION_CLOSED(nresult) ) {
			notifyClosed();
        }
    }
    else {
        log_error(L_NETSERV, "[netserv_con(%d)] connection is not open\n", nSessId);
		m_pParent->statFail();

		notifySend(ENOTCONN, nSessId);
    }
}

void CNetServerConnection::notifyRecv(CNetContainer* pContainer)
{
	CEventNetServerRecv*    pEvent;

	pEvent = new CEventNetServerRecv(m_pParent->getReceiver(), pContainer, this);
	appSendEvent(pEvent);
}

/*
 * Receive a container with predefined timeout
 *
 * Result: send EV_NET_SERVER_RECV notification on receive any container
 */
result_t CNetServerConnection::doRecv(hr_time_t htTimeout)
{
	dec_ptr<CNetContainer>	pContainerOriginal, pContainer;
    result_t                nresult = ENOTCONN;

	pContainerOriginal = getRecvTemplRef();
    shell_assert((CNetContainer*)pContainerOriginal);

    try {
		pContainer = pContainerOriginal->clone();
    }
	catch (const std::bad_alloc& exc)  {
		log_error(L_NETSERV, "[netserv_con] memory allocation failed\n");
		m_pParent->statFail();
		return ENOMEM;
	}

    if ( m_pSocket->isOpen() )  {
		log_trace(L_NETSERV, "[netserv_con] receiving a container\n");
        nresult = pContainer->receive(*m_pSocket, htTimeout);

		if ( nresult == ESUCCESS || nresult == ETIMEDOUT || nresult == ECANCELED )  {
			m_bFailSent = FALSE;
		}

		if ( nresult == ESUCCESS )  {
			if ( logger_is_enabled(LT_TRACE|L_NETSERV_IO) ) {
				char    strTmp[128];

				pContainer->getDump(strTmp, sizeof(strTmp));
				log_trace(L_NETCONN_IO, "[netserv_con] >>> Recv container: %s\n", strTmp);
			}
			else {
				log_trace(L_NETSERV, "[netserv_con] received a container\n");
			}

			notifyRecv(pContainer);
			m_pParent->statRecv();
        }
        else {
            if ( nresult != ETIMEDOUT && nresult != ECANCELED )  {
				if ( IS_NETWORK_CONNECTION_CLOSED(nresult) )  {
					log_trace(L_NETSERV, "[netserv_con] receive packet failed, error %d\n", nresult);
				}
				else {
					log_debug(L_NETSERV, "[netserv_con] receive packet failed, error %d\n", nresult);
				}

				m_pParent->statFail();
            }

			if ( IS_NETWORK_CONNECTION_CLOSED(nresult) ) {
				notifyClosed();
			}
        }
    }
	else {
		log_error(L_NETSERV, "[netserv_con] connection is not open\n");
		m_pParent->statFail();
	}

	return nresult;
}

/*
 * Notify server when a client closes connection,
 * send a "Connection closed by peer notification
 */
void CNetServerConnection::notifyClosed()
{
	if ( !m_bFailSent ) {
		CEvent*		pEvent;

		log_trace(L_NETSERV, "[netserv_con] connection closed, sending a notification\n");

		pEvent = new CEvent(EV_NET_SERVER_DISCONNECTED, m_pParent->getReceiver(),
							(PPARAM)getHandle(), (NPARAM)0, "net_serv_closed_connection");
		appSendEvent(pEvent);
		m_bFailSent = TRUE;
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
boolean_t CNetServerConnection::processEvent(CEvent* pEvent)
{
    CEventNetServerDoSend*  pEventSend;
    boolean_t               bProcessed = FALSE;

    switch ( pEvent->getType() )  {
        case EV_NET_SERVER_DO_SEND:
            pEventSend = dynamic_cast<CEventNetServerDoSend*>(pEvent);
            shell_assert(pEventSend);
            doSend(pEventSend->getContainer(), pEventSend->getSessId());

            bProcessed = TRUE;
            break;
    }

    return bProcessed;
}

void CNetServerConnection::notify()
{
	m_cond.lock();
    if ( m_pSocket->isOpen() && m_bRecvBreak )  {
        m_pSocket->breakerBreak();
    }
	m_cond.unlock();

    CEventLoop::notify();
}

result_t CNetServerConnection::waitReceive()
{
	hr_time_t   hrNextIterTime, hrTimeout, hrNow;
	result_t	nresult;

	if ( m_bDone )  {
		return ETIMEDOUT;
	}

	if ( !m_pSocket->isOpen() )  {
		return ENOTCONN;
	}

	nresult = ETIMEDOUT;
	if ( (hrNextIterTime = getNextIterTime()) != HR_0 ) {
		hrNow = hr_time_now();
		hrTimeout = hrNextIterTime > hrNow ? sh_min(hrNextIterTime-hrNow, HR_1MIN) : HR_0;

		if ( hrTimeout > HR_0 ) {
			m_bRecvBreak = TRUE;
			m_cond.unlock();

			//log_debug(L_NETSERV_FL, "[netserv_con] awaiting on IDLE...\n");
			nresult = m_pSocket->select(hrTimeout, CSocket::pollRead);

			m_cond.lock();
			m_bRecvBreak = FALSE;
		}
	}

	return nresult;
}

result_t CNetServerConnection::processReceive()
{
	hr_time_t	hrNow, hrNextIterTime, hrTimeout, hrRecvTimeout;
	result_t	nresult;

	if ( m_bDone || (hrNextIterTime = getNextIterTime()) == HR_0 ) {
		return ETIMEDOUT;
	}

	hrNow = hr_time_now();
	hrTimeout = hrNextIterTime > hrNow ? (hrNextIterTime-hrNow) : HR_0;

	m_cond.unlock();

	hrRecvTimeout = m_pParent->getRecvTimeout();
	if ( hrTimeout >= hrRecvTimeout )  {
		log_trace(L_NETSERV, "[netserv_con] receiving on IDLE...\n");
		nresult = doRecv(hrRecvTimeout);

		m_cond.lock();
		m_pSocket->breakerReset();
	}
	else {
		nresult = EFAULT;
		m_cond.lock();
	}

	return nresult;
}

void CNetServerConnection::runEventLoop()
{
	hr_time_t   hrNextIterTime, hrNow, hrDuration, hrDelay;
	result_t	nresult;

	m_bRecvBreak = FALSE;

	while ( !m_bDone )  {
		dispatchEvents();

		m_cond.lock();

		hrNow = hr_time_now();

		nresult = waitReceive();
		if ( nresult == ESUCCESS )  {
			nresult = processReceive();
		}

		hrDuration = hr_time_get_elapsed(hrNow);
		if ( (hrNextIterTime = getNextIterTime()) != HR_0 && !m_bDone &&
				nresult != ESUCCESS && nresult != ECANCELED && nresult != ETIMEDOUT &&
				hrDuration < HR_50MSEC )
		{
				hrDelay = IS_NETWORK_CONNECTION_CLOSED(nresult) ? HR_2MIN : HR_500MSEC;
				hrNextIterTime = sh_min(hrNextIterTime, hr_time_now()+hrDelay);

				/*log_debug(L_NETSERV_FL, "[netserv_con] sleeping %d ms...\n",
						  HR_TIME_TO_MILLISECONDS(hrNextIterTime-hr_time_now()));*/
				m_cond.waitTimed(hrNextIterTime);
		}

		m_cond.unlock();
	}
}

void CNetServerConnection::disconnect()
{
	if ( m_pSocket->isOpen() ) {
		m_pSocket->close();
	}
}


/*
 * Abort all operations on the client
 */
void CNetServerConnection::abort()
{
}

/*******************************************************************************/

/*
 * Client initialisation
 *
 * Return:
 *      ESUCCESS    successfully initialised
 *      other code  client is not initialised
 */
result_t CNetServerConnection::init()
{
    result_t    nresult;

	m_bRecvBreak = FALSE;
	m_bFailSent = FALSE;

    nresult = CEventLoopThread::start();
    if ( nresult != ESUCCESS )  {
        log_error(L_NETSERV, "[netserv_con] failed to start thread, result %d\n", nresult);
    }

    return nresult;
}

/*
 * Release connection resources and terminate worker thread
 */
void CNetServerConnection::terminate()
{
	CEventLoopThread::stop();
	disconnect();
	m_pSocket->close();
}
