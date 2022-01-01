/*
 *  Carbon framework
 *  UDP connector module
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 14.04.2017 11:53:56
 *      Initial revision.
 *
 */

#include <new>

#include "shell/utils.h"

#include "carbon/net_connector/udp_connector.h"

#define MODULE_NAME     "udpconn"

#define UDP_CONNECTOR_SEND_TIMEOUT			HR_2SEC
#define UDP_CONNECTOR_RECV_TIMEOUT			HR_1SEC


/*******************************************************************************
 * CUdpConnector Module
 */

CUdpConnector::CUdpConnector(CNetContainer* pRecvTempl, CEventReceiver* pParent) :
    CModule(MODULE_NAME),
    m_pParent(pParent),
	m_pRecvTempl(pRecvTempl),
	m_thread(MODULE_NAME),
	m_hrSendTimeout(UDP_CONNECTOR_SEND_TIMEOUT),
	m_bDone(ZERO_ATOMIC)
{
    shell_assert(pParent);
	shell_assert(pRecvTempl);
}

CUdpConnector::~CUdpConnector()
{
	shell_assert(!m_thread.isRunning());
	shell_assert(!m_socket.isOpen());
}

void CUdpConnector::resetStat()
{
	counter_reset_struct(m_stat);
}

void CUdpConnector::notifyReceive(CNetContainer* pContainer, const CNetAddr& srcAddr)
{
	CEventUdpRecv*		pEvent;

	pEvent = new CEventUdpRecv(m_pParent, pContainer, srcAddr);
	appSendEvent(pEvent);
}

void* CUdpConnector::workerThread(CThread* pThread, void* p)
{
	shell_unused(p);
	pThread->bootCompleted(ESUCCESS);

	shell_assert(m_socket.isOpen());

	while ( sh_atomic_get(&m_bDone) == 0 )  {
		dec_ptr<CNetContainer>	pContainer;
		CNetAddr				srcAddr;
		result_t				nresult;

		try {
			pContainer = m_pRecvTempl->clone();
			nresult = pContainer->receive(m_socket, UDP_CONNECTOR_RECV_TIMEOUT, &srcAddr);
			if ( nresult == ESUCCESS )  {
				if ( logger_is_enabled(LT_TRACE|L_NETCONN_IO) )  {
					char    strTmp[128];

					pContainer->getDump(strTmp, sizeof(strTmp));
					log_trace(L_NETCONN_IO, "[udpconn] >>> Recv container (from %s): %s\n",
							  srcAddr.cs(), strTmp);
				}
				else {
					log_trace(L_NETCONN, "[udpconn] container from %s is received\n", srcAddr.cs());
				}

				notifyReceive(pContainer, srcAddr);
				statRecv();
			}
			else {
				statFail();
				if ( sh_atomic_get(&m_bDone) == 0 && nresult != ECANCELED && nresult != ETIMEDOUT ) {
					log_error(L_GEN, "[udpconn] failed to receive container, result %d\n", nresult);
					sleep_s(1);
				}
			}
		}
		catch (const std::bad_alloc& exc)  {
			log_error(L_GEN, "[udpconn] memory allocation failed\n");
			statFail();
			sleep_s(1);
		}
	}

	return NULL;
}

/*
 * [Public API]
 *
 * Send a container
 *
 *      pContainer          container to send
 *      dstAddr				destination address
 *
 * Return:
 *      ESUCCESS        container was successfully queued for sending
 *      EINVAL          wrong parameters (container or socket)
 *      ENOMEM          memory shortage
 */
result_t CUdpConnector::sendSync(CNetContainer* pContainer, const CNetAddr& dstAddr)
{
	CSocket		socket((ip_addr_t)dstAddr == INADDR_BROADCAST ? CSocket::broadcast : 0);
	result_t    nresult;

	log_trace(L_NETCONN, "[udpconn] sending a container to '%s'\n", dstAddr.cs());

	nresult = socket.open(m_bindAddr, SOCKET_TYPE_UDP);
	if ( nresult == ESUCCESS )  {
		nresult = pContainer->send(socket, m_hrSendTimeout, dstAddr);
		if ( nresult == ESUCCESS ) {
			if ( logger_is_enabled(LT_TRACE|L_NETCONN_IO) )  {
				char    strTmp[128];

				pContainer->getDump(strTmp, sizeof(strTmp));
				log_trace(L_NETCONN_IO, "[udpconn] <<< Sent container (to %s): %s\n",
						  dstAddr.cs(), strTmp);
			}
			else {
				log_trace(L_NETCONN, "[udpconn] container to %s sent\n", dstAddr.cs());
			}

			statSend();
		}
		else {
			log_debug(L_NETCONN, "[udpconn] failed to send container to %s, result: %d\n",
					  dstAddr.cs(), nresult);
			statFail();
		}
	}
	else {
		statFail();
	}

	socket.close();

	return nresult;
}

/*
 * [Public API]
 *
 * Start accepting the incoming packets
 *
 * Return:
 *      ESUCCESS, ...
 */
result_t CUdpConnector::startListen(const CNetAddr& listenAddr)
{
	result_t    nresult;

	shell_assert(listenAddr.isValid());
	shell_assert(!m_socket.isOpen());

	if ( m_socket.isOpen() )  {
		log_debug(L_NETCONN, "[udpconn] server already started\n");
		return EBUSY;
	}

	m_listenAddr = listenAddr;
	nresult = m_socket.open(m_listenAddr, SOCKET_TYPE_UDP);
	if ( nresult != ESUCCESS )  {
		log_error(L_NETCONN, "[udpconn] can't open listening socket, result %d\n", nresult);
		return nresult;
	}

	m_socket.breakerEnable();

	nresult = m_thread.start(THREAD_CALLBACK(CUdpConnector::workerThread, this));
	if ( nresult != ESUCCESS )  {
		m_socket.close();
		return nresult;
	}

	return nresult;
}

/*
 * [Public API]
 *
 * Stop accepting the incoming packets
 */
void CUdpConnector::stopListen()
{
	sh_atomic_inc(&m_bDone);
	m_socket.breakerBreak();
	m_thread.stop();
	m_socket.close();
}

result_t CUdpConnector::init()
{
	result_t	nresult;

	shell_assert(!m_thread.isRunning());

	nresult = CModule::init();
	if ( nresult != ESUCCESS ) {
		return nresult;
	}

	sh_atomic_set(&m_bDone, 0);

	return nresult;
}

void CUdpConnector::terminate()
{
	sh_atomic_inc(&m_bDone);
	stopListen();
	CModule::terminate();
}

/*******************************************************************************
 * Debugging support
 */

#if CARBON_DEBUG_DUMP

void CUdpConnector::dump(const char* strPref) const
{
    log_dump("%sUdpConnector statistic:\n", strPref);
    log_dump("     received containers:     %d\n", counter_get(m_stat.recv));
    log_dump("     sent containers:         %d\n", counter_get(m_stat.send));
    log_dump("     I/O errors:              %d\n", counter_get(m_stat.fail));
}

#endif /* CARBON_DEBUG_DUMP */
