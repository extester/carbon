/*
 *  Carbon framework
 *  TCP Connector Listening server
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.06.2015 16:43:08
 *      Initial revision.
 */

#include <new>

#include "carbon/event.h"
#include "carbon/net_connector/tcp_connector.h"
#include "carbon/net_connector/tcp_worker.h"
#include "carbon/net_connector/tcp_listen.h"

#define MODULE_NAME			"tcp_conn_listen"


/*******************************************************************************
 * CTcpListenServer class
 */

CTcpListenServer::CTcpListenServer(CTcpConnector* pParent) :
    CTcpServer(MODULE_NAME),
    m_pParent(pParent),
    m_thServer(MODULE_NAME)
{
    shell_assert(pParent);
}

CTcpListenServer::~CTcpListenServer()
{
    shell_assert(!m_thServer.isRunning());
}

/*
 * Server worker thread
 */
void* CTcpListenServer::thread(CThread* pThread, void* pData)
{
    pThread->bootCompleted(ESUCCESS);

    log_debug(L_NETCONN_FL, "[tcpconn_listen] starting thread, listen on %s\n",
               m_strSocket.isEmpty() ? m_listenAddr.cs() : m_strSocket.cs());
    run();
    log_debug(L_NETCONN_FL, "[tcpconn_listen] thread has been stopped\n");

    return NULL;
}

/*
 * Process client connection
 *
 *      pSocket     client socket
 *
 * Return:
 *      ESUCCESS            everything is ok
 *      ENOMEM              memory shortage
 *      EINTR/ECANCELED     cancelled by user
 */
result_t CTcpListenServer::processClient(CSocketRef* pSocket)
{
    CTcpWorkerItem*    	pWorker;
    result_t            nresult = ESUCCESS;

    pWorker = m_pParent->getWorker();
    if ( pWorker )  {
        CEventTcpConnDoConnect*     pEvent = 0;

        try {
            pEvent = new CEventTcpConnDoConnect(pWorker, pSocket);
            pWorker->sendEvent(pEvent);
            m_pParent->statClient();
        }
        catch(const std::bad_alloc& exc) {
            log_error(L_NETCONN, "[tcpconn_listen] failed to allocate event\n");
            SAFE_RELEASE(pEvent);
            nresult = ENOMEM;
            m_pParent->statClientFail();
        }
    }
    else {
        log_error(L_NETCONN, "[tcpconn_listen] no available workers found\n");
        nresult = EFAULT;
        m_pParent->statClientFail();
    }

    return nresult;
}


/*
 * Start lisnen server
 *
 * Return: ESUCCESS, ...
 */
result_t CTcpListenServer::start()
{
    result_t	nresult;

	shell_assert(m_listenAddr.isValid() || !m_strSocket.isEmpty());

	nresult = m_thServer.start(THREAD_CALLBACK(CTcpListenServer::thread, this));
    if ( nresult != ESUCCESS )  {
        log_error(L_NETCONN, "[tcpconn_listen] failed to start server, result: %d\n", nresult);
    }

    return nresult;
}

/*
 * Stop listen server
 */
void CTcpListenServer::stop()
{
    CTcpServer::stop();
    m_thServer.stop();
}
