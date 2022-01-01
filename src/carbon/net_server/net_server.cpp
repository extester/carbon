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
 *  Revision 1.0, 23.06.2016 15:01:58
 *      Initial revision.
 */

#include <new>

#include "carbon/carbon.h"
#include "carbon/event.h"
#include "carbon/sync.h"
#include "carbon/net_server/net_server_connection.h"
#include "carbon/net_server/net_server.h"

#define MODULE_NAME			"net_server"

/*******************************************************************************
 * CSyncResult
 */

class CNetServerSyncResult : public CSyncBase
{
	public:
		CNetServerSyncResult(seqnum_t sessId) : CSyncBase(sessId)
		{
		}

		virtual ~CNetServerSyncResult()
		{
		}

	public:
		virtual result_t processSyncEvent();
};

result_t CNetServerSyncResult::processSyncEvent()
{
	result_t		nresult;

	if ( (CEvent*)m_pEvent == 0 )  {
		return ETIMEDOUT;
	}

	if ( m_pEvent->getType() != EV_NET_SERVER_SENT )  {
		log_debug(L_NETCONN, "[netserv] unexpected event type %d\n",  m_pEvent->getType());
		return EINVAL;
	}

	nresult = (result_t)m_pEvent->getnParam();
	return nresult;
}

/*******************************************************************************
 * CNetServer Module
 */
CNetServer::CNetServer(size_t nMaxConnection, CNetContainer* pRecvTempl, CEventReceiver* pParent) :
    CModule(MODULE_NAME),
    CTcpServer(MODULE_NAME),
    m_pParent(pParent),
    m_listenThread(MODULE_NAME),
    m_nMaxConnection(nMaxConnection),
	m_pRecvTempl(pRecvTempl),
    m_hrSendTimeout(NET_SERVER_SEND_TIMEOUT),
	m_hrRecvTimeout(NET_SERVER_RECV_TIMEOUT)
{
    m_arConnection.reserve(nMaxConnection);
    shell_assert(pParent);
}

CNetServer::~CNetServer()
{
    shell_assert(m_arConnection.empty());
}

/*
 * Reset server statistic
 */
void CNetServer::resetStat()
{
    counter_reset_struct(m_stat);
}

/*
 * Listener thread worker
 */
void* CNetServer::threadListen(CThread* pThread, void* pData)
{
    pThread->bootCompleted(ESUCCESS);

    log_debug2(L_BOOT, L_NETSERV, "[netserv] starting listen thread '%s' on %s\n",
              pThread->getName(), servAddrStr());
    run();
    log_debug2(L_BOOT, L_NETSERV, "[netserv] listen thread '%s' has been stopped\n",
              pThread->getName());

    return NULL;
}

/*
 * Called when a client connection was established
 *
 *      pSocket         connected socket
 *
 * Return: ESUCCESS, ...
 */
result_t CNetServer::processClient(CSocketRef* pSocket)
{
    CAutoLock   locker(m_lock);
    size_t      count;
    result_t    nresult = E2BIG;

    count = m_arConnection.size();
    locker.unlock();

    if ( count < m_nMaxConnection )  {
        CNetServerConnection*   pConnection;

		log_trace(L_NETSERV, "[netserv] accepted client\n");
        pConnection = createConnection(pSocket);
        if ( pConnection )  {
            CEvent*     pEvent;

            pEvent = new CEvent(EV_NET_SERVER_CONNECTED, m_pParent,
						(PPARAM)pConnection->getHandle(), 0, "net_serv_connect");
            appSendEvent(pEvent);
            statClient();
            nresult = ESUCCESS;
        }
        else {
            statClientFail();
            nresult = ENOMEM;
        }
    }
    else {
        log_debug(L_NETSERV, "[netserv] too many clients (%lu), client dropped\n", count);
        statClientFail();
    }

    return nresult;
}

/*
 * Find a specific connection within server connections
 *
 *      pConnection     connection to search
 *
 * Return: connection index or -1 if not found
 *
 * Note: net server lock must be held
 */
int CNetServer::findConnection(CNetServerConnection* pConnection) const
{
    int     i, count = (int)m_arConnection.size();
    int     index = -1;

    for(i=0; i<count; i++)  {
        if ( m_arConnection[i] == pConnection )  {
            index = i;
            break;
        }
    }

    return index;
}

/*
 * Create new connection object in the server
 *
 *      pSocket         connected socket
 *
 * Return: Connection pointer or 0
 */
CNetServerConnection* CNetServer::createConnection(CSocketRef* pSocket)
{
    CNetServerConnection*   pConnection = 0;

    shell_assert(pSocket->isOpen());

    try {
        result_t    nresult;

        pConnection = new CNetServerConnection(pSocket, this);

		CAutoLock	locker(m_lock);
		m_arConnection.push_back(pConnection);
		locker.unlock();

        nresult = pConnection->init();
        if ( nresult == ESUCCESS )  {
			statConnection(m_arConnection.size());
        }
        else {
			CAutoLock	locker(m_lock);
			int 		index;

			index = findConnection(pConnection);
			if ( index >= 0 ) {
				m_arConnection.erase(m_arConnection.begin()+index);
			}

			locker.unlock();
			SAFE_DELETE(pConnection);
        }
    }
    catch(const std::bad_alloc& exc) {
        SAFE_DELETE(pConnection);
    }

    return pConnection;
}

/*
 * Delete a specified connection
 */
result_t CNetServer::deleteConnection(CNetServerConnection* pConnection)
{
    CAutoLock   locker(m_lock);
    int         index;
    result_t    nresult = ESUCCESS;

    index = findConnection(pConnection);
    if ( index >= 0 )  {
        m_arConnection.erase(m_arConnection.begin()+index);
		statConnection(m_arConnection.size());
        locker.unlock();

        //pConnection->abort();
		//pConnection->disconnect();
        pConnection->terminate();
        delete pConnection;
    }
    else {
        log_error(L_NETSERV, "[netserv] connection is not found %lu\n", (natural_t)pConnection);
        nresult = ENOENT;
    }

    return nresult;
}

/*
 * [Public API]
 *
 * Start accepting an incoming connections
 *
 * Return:
 *      ESUCCESS, ...
 */
result_t CNetServer::startListen(const CNetAddr& listenAddr)
{
	CNetAddr	curListenAddr;
    result_t    nresult = ESUCCESS;

	shell_assert((CNetContainer*)m_pRecvTempl != 0);

	getAddr(curListenAddr);
	if ( !m_listenThread.isRunning() || listenAddr != curListenAddr )  {
		stopListen();

		setAddr(listenAddr);
		nresult = m_listenThread.start(THREAD_CALLBACK(CNetServer::threadListen, this));
		if ( nresult != ESUCCESS )  {
			log_error(L_NETSERV, "[netserv] failed to start listen thread, result %d\n", nresult);
		}
	}

    return nresult;
}

/*
 * [Public API]
 *
 * Stop accepting an incoming connections
 */
void CNetServer::stopListen()
{
	stop();
    m_listenThread.stop();
}

/*
 * [Public API]
 *
 * Check if a connection is connected
 *
 * Return:
 * 		ESUCCESS		connected
 * 		ENOTCONN		is not connected
 * 		EINVAL			no connection found
 */
result_t CNetServer::isConnected(net_connection_t hConnection)
{
    CAutoLock       locker(m_lock);
    result_t		nresult = EINVAL;
    int             i;

    i = findConnection(hConnection);
    if ( i >= 0 )  {
        nresult = m_arConnection[i]->isConnected() ? ESUCCESS : ENOTCONN;
    }

    return nresult;
}

/*
 * [Public API]
 *
 *  Send a container using connected socket
 *
 *      pContainer      container to send
 *      hConnection     connection to send
 *      sessId          unique session id
 */
result_t CNetServer::send(CNetContainer* pContainer, net_connection_t hConnection,
                          seqnum_t sessId)
{
    CAutoLock       locker(m_lock);
    int             i;
    result_t        nresult = ESUCCESS;

    i = findConnection(hConnection);
    if ( i >= 0 )  {
        CEventNetServerDoSend*  pEvent;

        pEvent = new CEventNetServerDoSend(pContainer, m_arConnection[i], sessId);
        appSendEvent(pEvent);
    }
    else {
        log_error(L_NETSERV, "[netserv(%d)] connection %Xh is not found\n",
                  sessId, hConnection);
        nresult = ESRCH;
		statFail();
    }

    return nresult;
}

result_t CNetServer::sendSync(CNetContainer* pContainer, net_connection_t hConnection)
{
	seqnum_t				sessId = getUniqueId();
	CNetServerSyncResult	syncer(sessId);
	CEventLoop*				pEventLoop;
	result_t				nresult;

	pEventLoop = m_pParent->getEventLoop();
	if ( pEventLoop ) {
		pEventLoop->attachSync(&syncer);
		nresult = send(pContainer, hConnection, sessId);
		if ( nresult == ESUCCESS ) {
			nresult = pEventLoop->waitSync(getSendTimeout() + HR_30SEC);
		}
		pEventLoop->detachSync();
	}
	else {
		log_error(L_GEN, "[netserv] event loop is not found\n");
		nresult = ESRCH;
		statFail();
	}

	return nresult;
}

/*
 * [Public API]
 *
 *  Terminate connection
 *
 *      hConnection         connection handle
 */
result_t CNetServer::disconnect(net_connection_t hConnection)
{
    result_t    nresult;

    nresult = deleteConnection(hConnection);
    return nresult;
}

void CNetServer::closeConnections()
{
	m_lock.lock();
	while ( !m_arConnection.empty() )  {
		CNetServerConnection*   pConnection = m_arConnection[0];

		m_lock.unlock();
		deleteConnection(pConnection);
		m_lock.lock();
	}
	m_lock.unlock();
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
result_t CNetServer::init()
{
    result_t    nresult;

    nresult = CModule::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    resetStat();
    return ESUCCESS;
}

/*
 * [Initialisation API]
 *
 * Cancel all I/O operations, stop workers and listener server
 */
void CNetServer::terminate()
{
    stopListen();
	closeConnections();

	shell_assert_ex(m_pRecvTempl->getRefCount() == 1,
					"refcount = %d\n", m_pRecvTempl->getRefCount());
    CModule::terminate();
}

/*******************************************************************************
 * Debugging support
 */

void CNetServer::dump(const char* strPref) const
{
    log_dump("%sNetServer statistic:\n", strPref);
    log_dump("     clients:                 %d\n", counter_get(m_stat.client));
    log_dump("     client errors:           %d\n", counter_get(m_stat.client_fail));
    log_dump("     received containers:     %d\n", counter_get(m_stat.recv));
    log_dump("     sent containers:         %d\n", counter_get(m_stat.send));
    log_dump("     I/O errors:              %d\n", counter_get(m_stat.fail));
	log_dump("     connected clients:       %d of %u\n",
			 	counter_get(m_stat.connection), m_nMaxConnection);
}
