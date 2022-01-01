/*
 *  Carbon framework
 *  TCP server base class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 07.05.2015 20:59:57
 *      Initial revision.
 */

#include "shell/hr_time.h"
#include "shell/dec_ptr.h"
#include "shell/error.h"

#include "carbon/utils.h"
#include "carbon/logger.h"
#include "carbon/tcp_server.h"

#define TCP_SERVER_LISTEN_QUEUE_MAX			1024

/*******************************************************************************
 * CTcpServer class
 */

CTcpServer::CTcpServer(const char* strName) :
	CObject(strName),
	m_servAddr(NETADDR_NULL)
{
	m_pSocket = new CSocketRef();
	sh_atomic_set(&m_nStop, 0);
}

CTcpServer::~CTcpServer()
{
	m_pSocket->close();
	m_pSocket->release();
}

result_t CTcpServer::run()
{
	result_t	nresult;

	shell_assert(!m_pSocket->isOpen());

	m_pSocket->close();
	sh_atomic_set(&m_nStop, 0);

	if ( !isAddrLocal() ) {
		/*
		 * Specified an ip address to listen on
		 */
		nresult = m_pSocket->open(m_servAddr, (socket_type_t)(SOCKET_TYPE_STREAM|SOCKET_TYPE_CLOEXEC));
	}
	else {
		/*
		 * Specified an UNIX local socket path to listen on
		 */
		nresult = m_pSocket->open(m_strSocket, (socket_type_t)(SOCKET_TYPE_STREAM|SOCKET_TYPE_CLOEXEC));
	}

	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[tcp_server] failed to open socket %s, result %d\n",
				  servAddrStr(), nresult);
		return nresult;
	}

	nresult = m_pSocket->listen(TCP_SERVER_LISTEN_QUEUE_MAX);
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[tcp_server] failed to listen on %s, max connections %d, "
							"result %d\n", servAddrStr(), TCP_SERVER_LISTEN_QUEUE_MAX, nresult);
		m_pSocket->close();
		return nresult;
	}

	/*
	 * Processing connections
	 */

	while ( !isStopping() )  {
		nresult = m_pSocket->select(HR_1MIN, CSocket::pollRead);
		if ( nresult == ETIMEDOUT ) {
			continue;
		}

		if ( nresult == ESUCCESS )  {
			dec_ptr<CSocketRef>		clientSocketPtr;

			clientSocketPtr = m_pSocket->accept();
			if ( (CSocketRef*)clientSocketPtr )  {
				nresult = processClient(clientSocketPtr);
			}
		}

		if ( nresult == EINTR || nresult == ECANCELED )  {
			nresult = ESUCCESS;
			break;
		}

		if ( nresult != ESUCCESS )  {
			log_debug(L_GEN, "[tcp_server] select failed, result %d\n", nresult);
			hr_sleep(HR_1SEC);
		}
	}

	m_pSocket->close();
	return nresult;
}
