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
 *  Revision 1.0, 24.06.2016 21:27:26
 *      Initial revision.
 */

#include "carbon/carbon.h"
#include "carbon/sync.h"
#include "carbon/net_server/net_client.h"

#define MODULE_NAME                     "net_client"

#define NET_CLIENT_CONNECT_TIMEOUT      HR_8SEC
#define NET_CLIENT_SEND_TIMEOUT         HR_4SEC
#define NET_CLIENT_RECV_TIMEOUT         HR_10SEC


/*******************************************************************************
 * CNetClientSyncConnect
 */

class CNetClientSyncConnect : public CSyncBase
{
	public:
		CNetClientSyncConnect(seqnum_t sessId) : CSyncBase(sessId)
		{
		}

		virtual ~CNetClientSyncConnect()
		{
		}

	public:
		virtual result_t processSyncEvent();
};

result_t CNetClientSyncConnect::processSyncEvent()
{
	result_t		nresult;

	if ( (CEvent*)m_pEvent == 0 )  {
		return ETIMEDOUT;
	}

	if ( m_pEvent->getType() != EV_NET_CLIENT_CONNECTED )  {
		log_debug(L_NETCLI, "[netcli] unexpected event type %d\n",  m_pEvent->getType());
		return EINVAL;
	}

	nresult = (result_t)m_pEvent->getnParam();
	return nresult;
}

/*******************************************************************************
 * CNetClientSyncDisconnect
 */

class CNetClientSyncDisconnect : public CSyncBase
{
	public:
		CNetClientSyncDisconnect(seqnum_t sessId) : CSyncBase(sessId)
		{
		}

		virtual ~CNetClientSyncDisconnect()
		{
		}

	public:
		virtual result_t processSyncEvent();
};

result_t CNetClientSyncDisconnect::processSyncEvent()
{
	if ( (CEvent*)m_pEvent == 0 )  {
		return ETIMEDOUT;
	}

	if ( m_pEvent->getType() != EV_NET_CLIENT_DISCONNECTED )  {
		log_debug(L_NETCLI, "[netcli] unexpected event type %d\n",  m_pEvent->getType());
		return EINVAL;
	}

	return ESUCCESS;
}


/*******************************************************************************
 * CNetClientSyncResult
 */

class CNetClientSyncResult : public CSyncBase
{
	public:
		CNetClientSyncResult(seqnum_t sessId) : CSyncBase(sessId)
		{
		}

		virtual ~CNetClientSyncResult()
		{
		}

	public:
		virtual result_t processSyncEvent();
};

result_t CNetClientSyncResult::processSyncEvent()
{
	result_t		nresult;

	if ( (CEvent*)m_pEvent == 0 )  {
		return ETIMEDOUT;
	}

	if ( m_pEvent->getType() != EV_NET_CLIENT_SENT )  {
		log_debug(L_NETCLI, "[netcli] unexpected event type %d\n",  m_pEvent->getType());
		return EINVAL;
	}

	nresult = (result_t)m_pEvent->getnParam();
	return nresult;
}

/*******************************************************************************
 * CNetClientSyncRecv
 */

class CNetClientSyncRecv : public CSyncBase
{
	public:
		CNetClientSyncRecv(seqnum_t sessId) : CSyncBase(sessId)
		{
		}

		virtual ~CNetClientSyncRecv()
		{
		}

	public:
		virtual result_t processSyncEvent();
};

result_t CNetClientSyncRecv::processSyncEvent()
{
	result_t		nresult;

	if ( (CEvent*)m_pEvent == 0 )  {
		return ETIMEDOUT;
	}

	if ( m_pEvent->getType() == EV_NET_CLIENT_RECV )  {
		/* IO successful */
		CEventNetClientRecv*	pEventRecv = dynamic_cast<CEventNetClientRecv*>((CEvent*)m_pEvent);

		shell_assert(pEventRecv);
		nresult = pEventRecv->getResult();
	}
	else {
		/* Wrong event */
		log_error(L_NETCLI, "[netcli] unexpected event type %d\n", m_pEvent->getType());
		nresult = EINVAL;
	}

	return nresult;
}


/*******************************************************************************
 * CNetClient Module
 */

CNetClient::CNetClient(CEventReceiver* pParent) :
    CModule(MODULE_NAME),
    CEventLoopThread(MODULE_NAME),
    CEventReceiver(this, MODULE_NAME),
    m_pParent(pParent),
    m_bindAddr(NETADDR_NULL),
    m_hrConnectTimeout(NET_CLIENT_CONNECT_TIMEOUT),
    m_hrSendTimeout(NET_CLIENT_SEND_TIMEOUT),
    m_hrRecvTimeout(NET_CLIENT_RECV_TIMEOUT)
{
    shell_assert(pParent);
}

CNetClient::~CNetClient()
{
    shell_assert(!m_socket.isOpen());
    //shell_assert(!CEventLoopThread::isRunning());
}

/*
 * Reset client statistic
 */
void CNetClient::resetStat()
{
    counter_reset_struct(m_stat);
}

/*
 * Connect to the remove server with predefined timeout
 *
 *      netAddr         address to connect
 *
 * Result: send EV_NET_CLIENT_CONNECTED notification
 */
void CNetClient::doConnect(const CNetAddr& netAddr, seqnum_t sessId, socket_type_t sockType)
{
    CAutoLock       locker(m_lock);
    CEvent*         pEvent;
    result_t        nresult;
    hr_time_t       hrTimeout = m_hrConnectTimeout;
    CNetAddr        bindAddr = m_bindAddr;

    locker.unlock();
    shell_assert(!m_socket.isOpen());
    m_socket.close();

    log_debug(L_NETCLI_FL, "[netcli(%d)] connecting to %s\n", sessId, netAddr.cs());

	if ( netAddr.isValid() )  {
	    nresult = m_socket.connect(netAddr, hrTimeout, bindAddr, sockType);
		if ( nresult == ESUCCESS )  {
			log_debug(L_NETCLI_FL, "[netcli(%d)] connected to %s\n", sessId, netAddr.cs());
			statConnect();
		}
		else {
			statFail();
		}
	}
	else {
		/* Destination address is not specified */
		log_error(L_NETCLI, "[netcli(%d)] distination address is not valid\n", sessId);
		statFail();
		nresult = EINVAL;
	}

    pEvent = new CEvent(EV_NET_CLIENT_CONNECTED, m_pParent, 0, (NPARAM)nresult,
						"net_client_connected");
    pEvent->setSessId(sessId);

    appSendEvent(pEvent);
}

void CNetClient::doConnect(const char* strSocket, seqnum_t sessId, socket_type_t sockType)
{
	CAutoLock       locker(m_lock);
	CEvent*         pEvent;
	result_t        nresult;
	hr_time_t       hrTimeout = m_hrConnectTimeout;
	CString			strBindAddr = m_strBindAddr;

	locker.unlock();
	shell_assert(!m_socket.isOpen());
	m_socket.close();

	log_debug(L_NETCLI_FL, "[netcli(%d)] connecting to %s\n", sessId, strSocket);

	nresult = m_socket.connect(strSocket, hrTimeout,
							   	strBindAddr.isEmpty() ? NULL : strBindAddr, sockType);
	if ( nresult == ESUCCESS )  {
		log_debug(L_NETCLI_FL, "[netcli(%d)] connected to %s\n", sessId, strSocket);
		statConnect();
	}
	else {
		statFail();
	}

	pEvent = new CEvent(EV_NET_CLIENT_CONNECTED, m_pParent, 0, (NPARAM)nresult,
						"net_client_connected");
	pEvent->setSessId(sessId);

	appSendEvent(pEvent);
}

/*
 * Disconnect connection
 *
 * Result: none
 */
void CNetClient::doDisconnect(seqnum_t sessId)
{
    log_debug(L_NETCLI_FL, "[netcli] disconnecting\n");

    //shell_assert(m_socket.isOpen());
    m_socket.close();

	if ( sessId != NO_SEQNUM )  {
		CEvent*		pEvent;

		pEvent = new CEvent(EV_NET_CLIENT_DISCONNECTED, m_pParent, 0, 0, "net_client_disconnect");
		pEvent->setSessId(sessId);

		appSendEvent(pEvent);
	}
}

/*
 * Send a container to the connected remote server with predefined timeout
 *
 *      pContainer      container to send
 *      sessId          unique session Id (NO_SEQNUM - do not send a reply)
 *
 * Result: send EV_NET_CLIENT_SENT notification
 */
void CNetClient::doSend(CNetContainer* pContainer, seqnum_t sessId)
{
    CAutoLock       locker(m_lock);
    result_t        nresult;
    hr_time_t       hrTimeout = m_hrSendTimeout;

    locker.unlock();
    shell_assert(pContainer);

    if ( m_socket.isOpen() ) {
		log_debug(L_NETCLI_FL, "[netcli(%d)] sending a container...\n", sessId);
        nresult = pContainer->send(m_socket, hrTimeout);
        if ( nresult == ESUCCESS ) {
			statSend();

			if ( logger_is_enabled(LT_DEBUG|L_NETCLI_IO) )  {
				char    strTmp[128];

				pContainer->getDump(strTmp, sizeof(strTmp));
				log_debug(L_NETCLI_IO, "[netcli] <<< Sent container: %s\n", strTmp);
			}
			else {
				log_debug(L_NETCLI_FL, "[netcli(%d)] container sent.\n", sessId);
			}
		}
		else {
            log_debug(L_NETCLI, "[netcli(%d)] send container failed, error %d\n",
                      sessId, nresult);
			statFail();
        }
    }
    else {
        log_error(L_NETCLI, "[netcli(%d)] can't send, is not connected\n", sessId);
        nresult = ENOTCONN;
		statFail();
    }

    if ( sessId != NO_SEQNUM ) {
        CEvent*         pEvent;

        pEvent = new CEvent(EV_NET_CLIENT_SENT, m_pParent, (PPARAM)0,
                            (NPARAM)nresult, "net_client_sent");
        pEvent->setSessId(sessId);
        appSendEvent(pEvent);
    }
}

/*
 * Receive a container to the remote server with predefined timeout
 *
 *      pContainer      container to receive data (defined type of the received data)
 *      sessId          unique session Id (may not be NO_SEQNUM)
 *
 * Result: send EV_NET_CLIENT_RECV notification
 */
void CNetClient::doRecv(CNetContainer* pContainer, seqnum_t sessId)
{
    CAutoLock               locker(m_lock);
    CEventNetClientRecv*    pEvent;
    result_t                nresult;
    hr_time_t               hrTimeout = m_hrRecvTimeout;

	shell_assert(pContainer);
	shell_assert(sessId != NO_SEQNUM);

    locker.unlock();
    if ( m_socket.isOpen() )  {
		log_debug(L_NETCLI_FL, "[netcli(%d)] receiving a container...\n", sessId);

        nresult = pContainer->receive(m_socket, hrTimeout);
		if ( nresult == ESUCCESS ) {
			statRecv();

			if ( logger_is_enabled(LT_DEBUG|L_NETCLI_IO) )  {
				char    strTmp[128];

				pContainer->getDump(strTmp, sizeof(strTmp));
				log_debug(L_NETCLI_IO, "[netcli] >>> Recv container: %s\n", strTmp);
			}
			else {
				log_debug(L_NETCLI_FL, "[netcli(%d)] container received.\n", sessId);
			}
		}
		else {
			if ( nresult != ETIMEDOUT ) {
				log_error(L_NETCLI, "[netcli(%d)] receive failed, result %d\n",
						  sessId, nresult);
				statFail();
			}
		}

        pEvent = new CEventNetClientRecv(nresult,
                                nresult == ESUCCESS ? &*pContainer : NULL,
                                m_pParent, sessId);
    }
    else {
        log_error(L_NETCLI, "[netcli(%d)] connection is not open\n", sessId);
		statFail();

        pEvent = new CEventNetClientRecv(ENOTCONN, 0, m_pParent, sessId);
    }

    appSendEvent(pEvent);
}

/*
 * Send a container and receive a reply container
 *
 *      pContainer      container to send/recv [IN/OUT]
 *      sessId          sessionId //(may not be NO_SEQNUM)
 *
 * Result: send EV_NET_CLIENT_RECV notification
 */
void CNetClient::doIo(CNetContainer* pContainer, seqnum_t sessId)
{
    CAutoLock               locker(m_lock);
    CEventNetClientRecv*    pEvent;
    hr_time_t               hrSendTimeout = m_hrSendTimeout;
    hr_time_t               hrRecvTimeout = m_hrRecvTimeout;

	shell_assert(pContainer);
	//shell_assert(sessId != NO_SEQNUM);

	locker.unlock();
    if ( m_socket.isOpen() )  {
        result_t    nresult;

		log_debug(L_NETCLI_FL, "[netcli(%d)] executing I/O...\n", sessId);

        nresult = pContainer->send(m_socket, hrSendTimeout);
        if ( nresult == ESUCCESS )  {
			statSend();

			if ( logger_is_enabled(LT_DEBUG|L_NETCLI_IO) )  {
				char    strTmp[128];

				pContainer->getDump(strTmp, sizeof(strTmp));
				log_debug(L_NETCLI_IO, "[netcli] <<< I/O sent container: %s\n", strTmp);
			}
			else {
				log_debug(L_NETCLI_FL, "[netcli(%d)] i/o container sent.\n", sessId);
			}

            pContainer->clear();
            nresult = pContainer->receive(m_socket, hrRecvTimeout);
			if ( nresult == ESUCCESS )  {
				statRecv();

				if ( logger_is_enabled(LT_DEBUG|L_NETCLI_IO) )  {
					char    strTmp[128];

					pContainer->getDump(strTmp, sizeof(strTmp));
					log_debug(L_NETCLI_IO, "[netcli] >>> I/O recv container: %s\n", strTmp);
				}
				else {
					log_debug(L_NETCLI_FL, "[netcli(%d)] container received.\n", sessId);
				}
			}
			else {
				log_error(L_NETCLI, "[netcli(%d)] i/o receive failed, error %d\n",
						  sessId, nresult);
				statFail();
			}
        }
        else {
            log_error(L_NETCLI, "[netcli(%d)] i/o send failed, error %d\n",
                      sessId, nresult);
			statFail();
        }

        pEvent = new CEventNetClientRecv(nresult,
                            nresult == ESUCCESS ? &*pContainer : NULL,
                            m_pParent, sessId);
    }
    else {
        log_error(L_NETCLI, "[netcli(%d)] connection is not open\n", sessId);
		statFail();

        pEvent = new CEventNetClientRecv(ENOTCONN, 0, m_pParent, sessId);
    }

    appSendEvent(pEvent);
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
boolean_t CNetClient::processEvent(CEvent* pEvent)
{
    CEventNetClientDoConnect*   pEventConnect;
    CEventNetClientDoSend*      pEventSend;
    CEventNetClientDoRecv*      pEventRecv;
    CEventNetClientDoIo*        pEventIo;

	const net_client_do_connect_t*	pConnData;
    boolean_t                   bProcessed = FALSE;

    switch ( pEvent->getType() )  {
        case EV_NET_CLIENT_DO_CONNECT:
            pEventConnect = dynamic_cast<CEventNetClientDoConnect*>(pEvent);
            shell_assert(pEventConnect);
			pConnData = pEventConnect->getData();
			if ( pConnData->strSocket.size() != 0 )  {
				doConnect(pConnData->strSocket, pEventConnect->getSessId(),
						  	pConnData->sockType);
			}
			else {
				doConnect(pConnData->netAddr, pEventConnect->getSessId(),
							pConnData->sockType);
			}
            bProcessed = TRUE;
            break;

        case EV_NET_CLIENT_DO_DISCONNECT:
            doDisconnect(pEvent->getSessId());
            bProcessed = TRUE;
            break;

        case EV_NET_CLIENT_DO_SEND:
            pEventSend = dynamic_cast<CEventNetClientDoSend*>(pEvent);
            shell_assert(pEventSend);
            doSend(pEventSend->getContainer(), pEventSend->getSessId());
            bProcessed = TRUE;
            break;

        case EV_NET_CLIENT_DO_RECV:
            pEventRecv = dynamic_cast<CEventNetClientDoRecv*>(pEvent);
            shell_assert(pEventRecv);
            doRecv(pEventRecv->getContainer(), pEventRecv->getSessId());
            bProcessed = TRUE;
            break;

        case EV_NET_CLIENT_DO_IO:
			pEventIo = dynamic_cast<CEventNetClientDoIo*>(pEvent);
            shell_assert(pEventIo);
            doIo(pEventIo->getContainer(), pEventIo->getSessId());
            bProcessed = TRUE;
            break;

    }

    return bProcessed;
}

/*******************************************************************************/

/*
 * [Public API]
 *
 * Connect to the remote server
 *
 *      netAddr     address to connect
 *      sessId      unique session Id (may not be NO_SEQNUM)
 *      sockType	socket type, SOCKET_TYPE_xxx
 */
void CNetClient::connect(const CNetAddr& netAddr, seqnum_t sessId, socket_type_t sockType)
{
    CEventNetClientDoConnect*   pEvent;
    net_client_do_connect_t     data;

	shell_assert(sessId != NO_SEQNUM);

	data.netAddr = netAddr;
	data.sockType = sockType;

    pEvent = new CEventNetClientDoConnect(&data, this, netAddr.cs());
    pEvent->setSessId(sessId);

    appSendEvent(pEvent);
}

/*
 * [Public API]
 *
 * Connect to the remote server
 *
 *      strSocket	full filename of the local UNIX socket
 *      sessId      unique session Id (may not be NO_SEQNUM)
 *      sockType	socket type, SOCKET_TYPE_xxx
 */
void CNetClient::connect(const char* strSocket, seqnum_t sessId, socket_type_t sockType)
{
	CEventNetClientDoConnect*   pEvent;
	net_client_do_connect_t     data;

	shell_assert(sessId != NO_SEQNUM);

	data.strSocket = strSocket;
	data.sockType = sockType;

	pEvent = new CEventNetClientDoConnect(&data, this, strSocket);
	pEvent->setSessId(sessId);

	appSendEvent(pEvent);
}

/*
 * [Public API]
 *
 * Connect to the remote server synchronous
 *
 *      netAddr     address to connect
 *
 * Return: ESUCCESS, ...
 */
result_t CNetClient::connectSync(const CNetAddr& netAddr, socket_type_t sockType)
{
	seqnum_t				sessId = getUniqueId();
	CNetClientSyncConnect	syncer(sessId);
	CEventLoop*				pEventLoop;
	result_t				nresult;

	pEventLoop = m_pParent->getEventLoop();
	if ( pEventLoop ) {
		pEventLoop->attachSync(&syncer);
		connect(netAddr, sessId, sockType);
		nresult = pEventLoop->waitSync(getConnectTimeout() + HR_30SEC);
		pEventLoop->detachSync();
	}
	else {
		log_error(L_NETCLI, "[netcli] event loop is not found\n");
		nresult = ESRCH;
		statFail();
	}

	return nresult;
}

/*
 * [Public API]
 *
 * Disconnect from the remote server
 */
void CNetClient::disconnect(seqnum_t nSessId)
{
    CEvent*     pEvent;

    pEvent = new CEvent(EV_NET_CLIENT_DO_DISCONNECT, this, 0, 0, "net_client_do_disconnect");
	pEvent->setSessId(nSessId);
    appSendEvent(pEvent);
}

/*
 * [Public API]
 *
 * Disconnect from the remote server synchronous
 *
 * Return: ESUCCESS, ...
 */
result_t CNetClient::disconnectSync()
{
	seqnum_t					sessId = getUniqueId();
	CNetClientSyncDisconnect	syncer(sessId);
	CEventLoop*					pEventLoop;
	result_t					nresult;

	pEventLoop = m_pParent->getEventLoop();
	if ( pEventLoop ) {
		pEventLoop->attachSync(&syncer);
		disconnect(sessId);
		nresult = pEventLoop->waitSync(getConnectTimeout() + HR_30SEC);
		pEventLoop->detachSync();
	}
	else {
		log_error(L_NETCLI, "[netcli] event loop is not found\n");
		nresult = ESRCH;
		statFail();
	}

	return nresult;
}

/*
 * [Public API]
 *
 * Send a container
 *
 *      pContainer          container to send
 *      sessId              unique session Id
 */
void CNetClient::send(CNetContainer* pContainer, seqnum_t sessId)
{
    CEventNetClientDoSend*  pEvent;

    shell_assert(pContainer);
    pEvent = new CEventNetClientDoSend(pContainer, this, sessId);
    appSendEvent(pEvent);
}

result_t CNetClient::sendSync(CNetContainer* pContainer)
{
	seqnum_t				sessId = getUniqueId();
	CNetClientSyncResult	syncer(sessId);
	CEventLoop*				pEventLoop;
	result_t				nresult;

	pEventLoop = m_pParent->getEventLoop();
	if ( pEventLoop ) {
		hr_time_t	hrTimeout;

		getTimeouts(&hrTimeout, NULL);
		pEventLoop->attachSync(&syncer);
		send(pContainer, sessId);
		nresult = pEventLoop->waitSync(hrTimeout + HR_30SEC);
		pEventLoop->detachSync();
	}
	else {
		log_error(L_NETCLI, "[netcli] event loop is not found\n");
		nresult = ESRCH;
		statFail();
	}

	return nresult;
}


/*
 * [Public API]
 *
 * Awaiting for and receive a container
 *
 *      pContainer      container to receive
 *      sessId          unique session Id
 */
void CNetClient::recv(CNetContainer* pContainer, seqnum_t sessId)
{
    CEventNetClientDoRecv*  pEvent;

    pEvent = new CEventNetClientDoRecv(pContainer, this, sessId);
    appSendEvent(pEvent);
}

result_t CNetClient::recvSync(CNetContainer* pContainer)
{
	seqnum_t			sessId = getUniqueId();
	CNetClientSyncRecv	syncer(sessId);
	CEventLoop*			pEventLoop;
	result_t			nresult;

	pEventLoop = m_pParent->getEventLoop();
	if ( pEventLoop ) {
		hr_time_t	hrTimeout;

		getTimeouts(NULL, &hrTimeout);
		pEventLoop->attachSync(&syncer);
		send(pContainer, sessId);
		nresult = pEventLoop->waitSync(hrTimeout + HR_30SEC);
		pEventLoop->detachSync();
	}
	else {
		log_error(L_NETCLI, "[netcli] event loop is not found\n");
		nresult = ESRCH;
		statFail();
	}

	return nresult;
}

/*
 * [Public API]
 *
 * Send a container and receive a reply container
 *
 *      pContainer		container to send/receive
 *      sessId          unique session Id
 *
 * Note: a received container is returned in the same container
 */
void CNetClient::io(CNetContainer* pContainer, seqnum_t sessId)
{
    CEventNetClientDoIo*    pEvent;

    pEvent = new CEventNetClientDoIo(pContainer, this, sessId);
    appSendEvent(pEvent);
}

result_t CNetClient::ioSync(CNetContainer* pContainer)
{
	seqnum_t			sessId = getUniqueId();
	CNetClientSyncRecv	syncer(sessId);
	CEventLoop*			pEventLoop;
	result_t			nresult;

	pEventLoop = m_pParent->getEventLoop();
	if ( pEventLoop ) {
		hr_time_t	hrSendTimeout, hrRecvTimeout;

		getTimeouts(&hrSendTimeout, &hrRecvTimeout);
		pEventLoop->attachSync(&syncer);
		io(pContainer, sessId);
		nresult = pEventLoop->waitSync(hrSendTimeout + hrRecvTimeout + HR_30SEC);
		pEventLoop->detachSync();
	}
	else {
		log_error(L_NETCLI, "[netcli] event loop is not found\n");
		nresult = ESRCH;
		statFail();
	}

	return nresult;
}

/*
 * Cancel any running I/O operation and delete any pending I/O
 */
void CNetClient::cancel()
{

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
result_t CNetClient::init()
{
    result_t    nresult;

    nresult = CModule::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    resetStat();

    nresult = CEventLoopThread::start();
    if ( nresult != ESUCCESS )  {
        CModule::terminate();
    }

    return nresult;
}

/*
 * [Initialisation API]
 *
 * Cancel all I/O operations, stop workers and listener server
 */
void CNetClient::terminate()
{
    CEventLoopThread::stop();
	m_socket.close();
    CModule::terminate();
}

/*******************************************************************************
 * Debugging support
 */

void CNetClient::dump(const char* strPref) const
{
    log_dump("%sNetClient statistic:\n", strPref);
	log_dump("     connected count:         %d\n", counter_get(m_stat.connect));
    log_dump("     received containers:     %d\n", counter_get(m_stat.recv));
    log_dump("     sent containers:         %d\n", counter_get(m_stat.send));
    log_dump("     I/O errors:              %d\n", counter_get(m_stat.fail));
}
