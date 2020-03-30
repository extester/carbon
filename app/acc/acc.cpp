/*
 *  Carbon Framework Network Client
 *  Client (GTK version)
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 09.06.2015 14:59:01
 *      Initial revision.
 */

#include <gtk/gtk.h>

#include "carbon/event.h"
#include "vep/packet_system.h"
#include "shell/logger/logger_base.h"

#include "acc.h"

#define ACC_UPDATE_PERIOD		HR_3SEC

static const char* accEventTable[] = {
		"EV_HB_ENABLE",
		"EV_HOST_INSERT",
		"EV_HOST_UPDATE",
		"EV_HOST_REMOVE",
		"EV_MEMORY_STAT",
		"EV_NETCONN_STAT",
		"EV_LOG_CONTROL",
		"EV_LOG_CONTROL_CHECK"
};

CAccApp::CAccApp(int argc, char* argv[]) :
	CApplication(ACC_NAME, ACC_VERSION, 1, argc, argv),
	m_pNetConnector(0),
	m_pInterface(0),
	m_config(TRUE),
	m_pUpdateTimer(0),
	m_bCenterConnected(FALSE),
	m_sessVersion(NO_SEQNUM),
	m_sessHeartBeat(NO_SEQNUM),
	m_sessHostInsert(NO_SEQNUM),
	m_sessHostUpdate(NO_SEQNUM),
	m_sessHostDelete(NO_SEQNUM),
	m_sessMemStat(NO_SEQNUM),
	m_sessNetConnStat(NO_SEQNUM),
	m_sessLogCtl(NO_SEQNUM),
	m_sessLogEnable(NO_SEQNUM),

	m_nMemStatType(MEMORY_STAT_DISABLED),
	m_pMemStatTimer(0),
	m_nNetConnStatType(NETCONN_STAT_DISABLED),
	m_pNetConnStatTimer(0),
	m_nLogCtl(LOG_CONTROL_DISABLED)
{
	_tbzero_object(m_memStat);
	_tbzero_object(m_netconnStat);
}

CAccApp::~CAccApp()
{
	shell_assert(!m_pUpdateTimer);
	shell_assert(!m_pMemStatTimer);
	shell_assert(!m_pNetConnStatTimer);

	SAFE_DELETE(m_pNetConnector);
	SAFE_DELETE(m_pInterface);
}

/*
 * Enable/Disable data updating
 *
 * 		bEnable		TRUE: enable updating, FALSE: disable
 */
void CAccApp::switchHeartBeat(boolean_t bEnable)
{
	if ( bEnable )  {
		getMainLoop()->restartTimer(m_pUpdateTimer);
	}
	else {
		getMainLoop()->pauseTimer(m_pUpdateTimer);
	}

	log_debug(L_GEN, "[acc] updating is %s\n", bEnable ? "ENABLED" : "DISABLED");
}

result_t CAccApp::doRequest(CVepContainer* pContainer, seqnum_t* pSessId)
{
	result_t	nresult;

	*pSessId = getUniqueId();
	nresult = m_pNetConnector->io(pContainer, m_addrCenter, this, *pSessId);
	if ( nresult != ESUCCESS )  {
		*pSessId = NO_SEQNUM;
	}

	return nresult;
}

void CAccApp::requestData()
{
	if ( m_sessHeartBeat == NO_SEQNUM ) {
		dec_ptr<CVepContainer>	containerPtr;

		containerPtr = new CVepContainer(VEP_CONTAINER_CENTER, CENTER_PACKET_HEARTBEAT);
		doRequest(containerPtr, &m_sessHeartBeat);
	}
}

void CAccApp::requestCenterVersion()
{
	if ( m_sessVersion == NO_SEQNUM ) {
		dec_ptr<CVepContainer>	containerPtr;

		containerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM, SYSTEM_PACKET_VERSION);
		doRequest(containerPtr, &m_sessVersion);
	}
}

void CAccApp::requestHostInsert(const char* strHost, const CNetHost& ipHost)
{
	if ( m_sessHostInsert == NO_SEQNUM ) {
		dec_ptr<CVepContainer>	containerPtr;
		center_packet_insert_t 	insert;
		result_t               	nresult;

		log_debug(L_GEN, "--- inserting host: ip: %s, name: %s\n",
				  (const char*)ipHost, strHost);

		containerPtr = new CVepContainer(VEP_CONTAINER_CENTER);
		copyString(insert.strName, strHost, sizeof(insert.strName));
		insert.ipAddr = ipHost;

		nresult = containerPtr->insertPacket(CENTER_PACKET_INSERT, &insert, sizeof(insert));
		if ( nresult == ESUCCESS ) {
			if ( doRequest(containerPtr, &m_sessHostInsert) == ESUCCESS )  {
				m_pInterface->updateBusyState(TRUE);
			}
		}
	}
}

void CAccApp::requestHostUpdate(host_id_t id, const char* strHost, const CNetHost& ipHost)
{
	if ( m_sessHostUpdate == NO_SEQNUM )  {
		dec_ptr<CVepContainer>	containerPtr;
		center_packet_update_t 	update;
		result_t               	nresult;

		log_debug(L_GEN, "--- updating host: id: %u, ip: %s, name: %s\n",
				  id, (const char*)ipHost, strHost);

		containerPtr = new CVepContainer(VEP_CONTAINER_CENTER);
		update.id = id;
		copyString(update.strName, strHost, sizeof(update.strName));
		update.ipAddr = ipHost;

		nresult = containerPtr->insertPacket(CENTER_PACKET_UPDATE, &update, sizeof(update));
		if ( nresult == ESUCCESS ) {
			if ( doRequest(containerPtr, &m_sessHostUpdate) == ESUCCESS )  {
				m_pInterface->updateBusyState(TRUE);
			}
		}
	}
}

void CAccApp::requestHostDelete(host_id_t id)
{
	if ( m_sessHostDelete == NO_SEQNUM )  {
		dec_ptr<CVepContainer>	containerPtr;
		center_packet_remove_t	remove;
		result_t				nresult;

		log_debug(L_GEN, "-- deleting host id: %u\n", id);

		containerPtr = new CVepContainer(VEP_CONTAINER_CENTER);
		remove.id = id;

		nresult = containerPtr->insertPacket(CENTER_PACKET_REMOVE, &remove, sizeof(remove));
		if ( nresult == ESUCCESS )  {
			if ( doRequest(containerPtr, &m_sessHostDelete) == ESUCCESS )  {
				m_pInterface->updateBusyState(TRUE);
			}
		}
	}
}

void CAccApp::getLocalMemoryStat()
{
	CMemoryManager*	pMemoryManager = appMemoryManager();

	shell_assert(pMemoryManager);
	if ( pMemoryManager ) {
		pMemoryManager->getStat(&m_memStat, sizeof(m_memStat));
	}
}

void CAccApp::getExternalMemoryStat()
{
	if ( m_sessMemStat == NO_SEQNUM ) {
		dec_ptr<CVepContainer>	containerPtr;
		CNetAddr			extAddr((ip_addr_t)m_memStatExtAddr, (ip_port_t)m_addrCenter);
		result_t			nresult;

		containerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM, SYSTEM_PACKET_MEMORY_STAT);
		m_sessNetConnStat = getUniqueId();
		nresult = m_pNetConnector->io(containerPtr, extAddr, this, m_sessMemStat);
		if ( nresult != ESUCCESS )  {
			m_sessMemStat = NO_SEQNUM;
		}
	}
}

void CAccApp::updateMemoryStat()
{
	/*if ( atomic_get(&m_memStat.full_alloc_count) != 0 ) {*/
		m_pInterface->updateMemoryStat(m_nMemStatType, &m_memStat);
	/*}*/
}


void CAccApp::doMemoryStat(int type, ip_addr_t extAddr)
{
	_tbzero_object(m_memStat);
	m_sessMemStat = NO_SEQNUM;
	m_nMemStatType = type;

	switch ( type )  {
		case MEMORY_STAT_LOCAL:				/* Local telemetry */
			getLocalMemoryStat();
			updateMemoryStat();
			restartMemoryStatTimer();
			break;

		case MEMORY_STAT_CENTER:			/* Center telemetry */
			m_memStatExtAddr = m_addrCenter;
			getExternalMemoryStat();
			restartMemoryStatTimer();
			break;

		case MEMORY_STAT_EXTERNAL:			/* Special telemetry */
			m_memStatExtAddr.setHost(extAddr);
			getExternalMemoryStat();
			restartMemoryStatTimer();
			break;

		default:
			shell_assert_ex(FALSE, "wrong memory statistic type: %d\n", type);
			m_nMemStatType = MEMORY_STAT_DISABLED;
			/* fall down */

		case MEMORY_STAT_DISABLED:			/* Disable statistic */
			stopMemoryStatTimer();
			break;
	}
}

void CAccApp::getLocalNetConnStat()
{
	if ( m_pNetConnector )  {
		m_pNetConnector->getStat(&m_netconnStat, sizeof(m_netconnStat));
	}
}

void CAccApp::getExternalNetConnStat()
{
	if ( m_sessNetConnStat == NO_SEQNUM ) {
		dec_ptr<CVepContainer>	containerPtr;
		CNetAddr			extAddr((ip_addr_t)m_netconnStatExtAddr, (ip_port_t)m_addrCenter);
		result_t			nresult;

		containerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM, SYSTEM_PACKET_NETCONN_STAT);
		m_sessNetConnStat = getUniqueId();
		nresult = m_pNetConnector->io(containerPtr, extAddr, this, m_sessNetConnStat);
		if ( nresult != ESUCCESS )  {
			m_sessNetConnStat = NO_SEQNUM;
		}
	}
}


void CAccApp::updateNetConnStat()
{
	/*if ( counter_get(m_netconnStat.worker) != 0 ) {*/
		m_pInterface->updateNetConnStat(m_nNetConnStatType, &m_netconnStat);
	/*}*/
}


/*
 * Enable/Disable/Switch network connector statistic mode
 *
 * 		type		statistic type, NETCONN_STAT_xxx
 * 		extAddr		external address to use in mode NETCONN_STAT_EXTERNAL
 */
void CAccApp::doNetConnStat(int type, ip_addr_t extAddr)
{
	_tbzero_object(m_netconnStat);
	m_sessNetConnStat = NO_SEQNUM;
	m_nNetConnStatType = type;

	switch ( type )  {
		case NETCONN_STAT_LOCAL:			/* Local telemetry */
			getLocalNetConnStat();
			updateNetConnStat();
			restartNetConnStatTimer();
			break;

		case NETCONN_STAT_CENTER:			/* Center telemetry */
			m_netconnStatExtAddr = m_addrCenter;
			getExternalNetConnStat();
			restartNetConnStatTimer();
			break;

		case NETCONN_STAT_EXTERNAL:			/* Special telemetry */
			m_netconnStatExtAddr.setHost(extAddr);
			getExternalNetConnStat();
			restartNetConnStatTimer();
			break;

		default:
			shell_assert_ex(FALSE, "wrong netconn statistic type: %d\n", type);
			m_nNetConnStatType = NETCONN_STAT_DISABLED;
			/* fall down */

		case NETCONN_STAT_DISABLED:			/* Disable statistic */
			stopNetConnStatTimer();
			break;
	}
}

/*******************************************************************************
 * Log Control
 */

void CAccApp::getLocalLogControl(uint8_t* pChannel)
{
	result_t	nresult;

	nresult = logger_get_channel(I_DEBUG, pChannel, LOGGER_CHANNEL_MAX/8);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[accapp] failed to get I_DEBUG logger data, result: %d\n", nresult);
		bzero(pChannel, LOGGER_CHANNEL_MAX/8);
	}
}

void CAccApp::getExternalLogControl()
{
	if ( m_sessLogCtl == NO_SEQNUM ) {
		dec_ptr<CVepContainer>	containerPtr;
		CNetAddr			extAddr((ip_addr_t)m_logctlExtAddr, (ip_port_t)m_addrCenter);
		result_t			nresult;

		containerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM, SYSTEM_PACKET_GET_LOGGER_CHANNEL);
		m_sessLogCtl = getUniqueId();
		nresult = m_pNetConnector->io(containerPtr, extAddr, this, m_sessLogCtl);
		if ( nresult != ESUCCESS )  {
			m_sessLogCtl = NO_SEQNUM;
		}
	}
}

void CAccApp::updateLogControl(const uint8_t* pChannel)
{
	m_pInterface->updateLogControl(m_nLogCtl, pChannel);
}

/*
 * Get logger channel data
 *
 * 		type		logger data type, LOG_CONTROL_xxx
 * 		extAddr		external address to use in mode LOG_CONTROL_EXTERNAL
 */
void CAccApp::doLogControl(int type, ip_addr_t extAddr)
{
	uint8_t		channel[LOGGER_CHANNEL_MAX/8];

	m_sessLogCtl = NO_SEQNUM;
	m_nLogCtl = type;

	switch ( type )  {
		case LOG_CONTROL_LOCAL:			/* Local logger */
			getLocalLogControl(channel);
			updateLogControl(channel);
			break;

		case LOG_CONTROL_CENTER:		/* Center logger */
			m_logctlExtAddr = m_addrCenter;
			getExternalLogControl();
			break;

		case LOG_CONTROL_EXTERNAL:		/* Special logger */
			m_logctlExtAddr.setHost(extAddr);
			getExternalLogControl();
			break;

		default:
			shell_assert_ex(FALSE, "wrong log control type: %d\n", type);
			m_nLogCtl = LOG_CONTROL_DISABLED;
			/* fall down */

		case NETCONN_STAT_DISABLED:		/* Disable Logger Control */
			break;
	}
}

void CAccApp::doLogExternalEnable(unsigned int nChannel, boolean_t isOn)
{
	if ( m_sessLogEnable == NO_SEQNUM ) {
		dec_ptr<CVepContainer>	containerPtr;
		CNetAddr						extAddr((ip_addr_t)m_logctlExtAddr, (ip_port_t)m_addrCenter);
		system_packet_logger_enable_t	enable;
		result_t						nresult;

		enable.levChan = nChannel;
		enable.enable = (uint8_t)isOn;

		containerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM);
		containerPtr->insertPacket(SYSTEM_PACKET_LOGGER_ENABLE, &enable, sizeof(enable));

		m_sessLogEnable = getUniqueId();
		nresult = m_pNetConnector->io(containerPtr, extAddr, this, m_sessLogEnable);
		if ( nresult != ESUCCESS )  {
			m_sessLogEnable = NO_SEQNUM;
		}
	}
}

void CAccApp::doLogControlCheck(unsigned int nChannel, boolean_t isOn)
{
	switch ( m_nLogCtl )  {
		case LOG_CONTROL_LOCAL:				/* Local Logger Control */
			if ( isOn ) {
				logger_enable(nChannel);
			}
			else {
				logger_disable(nChannel);
			}
			break;

		case LOG_CONTROL_CENTER:			/* Center Logger Control */
		case LOG_CONTROL_EXTERNAL:			/* External Logger Control */
			doLogExternalEnable(nChannel, isOn);
			break;

		case LOG_CONTROL_DISABLED:
			break;

		default:
			shell_assert(FALSE);
	}
}

boolean_t CAccApp::processPacketCenter(CEventTcpConnRecv* pEventRecv)
{
	CVepContainer*		pContainer = dynamic_cast<CVepContainer*>(pEventRecv->getContainer());
	seqnum_t			sessId = pEventRecv->getSessId();
	CVepPacket*			pPacket = (*pContainer)[0];
	boolean_t			bProcessed = FALSE;

	switch ( pPacket->getType() )  {
		case CENTER_PACKET_HEARTBEAT_REPLY:
			if ( m_sessHeartBeat == sessId )  {
				union centerPacketArgs* 	args;
				int 						i, count = pPacket->getSize() / sizeof(heartbeat_t);
				heartbeat_t* 				pItem;

				args = (union centerPacketArgs*)(vep_packet_head_t*)*pPacket;
				pItem = args->heartbeat_reply.list;

				for(i=0; i<count; i++)  {
					m_pInterface->updateHost(pItem->id, pItem->strName, pItem->ipAddr, pItem->hrPing);
					pItem++;
				}

				/* Request Apollo Center version */
				if ( !m_bCenterConnected )  {
					m_bCenterConnected = TRUE;
					m_pInterface->updateNetState(TRUE);
					requestCenterVersion();
				}

				m_sessHeartBeat = NO_SEQNUM;
				bProcessed = TRUE;
			}
			break;

		case CENTER_PACKET_INSERT_REPLY:
			if ( m_sessHostInsert == sessId )  {
				union centerPacketArgs* 	args;

				args = (union centerPacketArgs*)(vep_packet_head_t*)*pPacket;
				if ( args->insert_reply.nresult == ESUCCESS ) {
					log_debug(L_GEN, "[accapp] inserted host ID=%u\n", args->insert_reply.id);
				}
				else {
					log_debug(L_GEN, "[accapp] insert host failed on remote, result: %d\n",
							  args->insert_reply.nresult);
				}

				m_sessHostInsert = NO_SEQNUM;
				m_pInterface->updateBusyState(FALSE);
				bProcessed = TRUE;
			}
			break;

		case CENTER_PACKET_RESULT:
			if ( m_sessHostUpdate == sessId )  {
				union centerPacketArgs* 	args;

				args = (union centerPacketArgs*)(vep_packet_head_t*)*pPacket;
				if ( args->result.nresult == ESUCCESS ) {
					log_debug(L_GEN, "[accapp] host successfully updated on center\n");
				}
				else {
					log_error(L_GEN, "[accapp] update host on center failed, result: %d\n",
							  args->result.nresult);
				}

				m_sessHostUpdate = NO_SEQNUM;
				m_pInterface->updateBusyState(FALSE);
				bProcessed = TRUE;
			} else
			if ( m_sessHostDelete == sessId )  {
				union centerPacketArgs* 	args;

				args = (union centerPacketArgs*)(vep_packet_head_t*)*pPacket;
				if ( args->result.nresult == ESUCCESS ) {
					log_debug(L_GEN, "[accapp] host deleted on center\n");
				}
				else {
					log_error(L_GEN, "[accapp] delete host on center failed, result: %d\n",
							  args->result.nresult);
				}

				m_sessHostDelete = NO_SEQNUM;
				m_pInterface->updateBusyState(FALSE);
				bProcessed = TRUE;
			}
			break;

		default:
			log_debug(L_GEN, "[accapp] unknown INFO_CONTAINER_CENTER packet %u, dropped\n",
					  pPacket->getType());
	}

	return bProcessed;
}

boolean_t CAccApp::processPacketSystem(CEventTcpConnRecv* pEventRecv)
{
	CVepContainer*		pContainer = dynamic_cast<CVepContainer*>(pEventRecv->getContainer());
	seqnum_t			sessId = pEventRecv->getSessId();
	CVepPacket*			pPacket = (*pContainer)[0];
	boolean_t			bProcessed = FALSE;

	switch ( pPacket->getType() )  {
		case SYSTEM_PACKET_VERSION_REPLY:
			if ( m_sessVersion == sessId )  {
				union systemPacketArgs*	args;
				char	stmp[128];

				args = (union systemPacketArgs*)pContainer->getPacketHead();
				snprintf(stmp, sizeof(stmp), "ver. %d.%d (library ver. %d.%d)",
						 VERSION_MAJOR(args->version_reply.version),
						 VERSION_MINOR(args->version_reply.version),
						 VERSION_MAJOR(args->version_reply.lib_version),
						 VERSION_MINOR(args->version_reply.lib_version));
				m_pInterface->updateCenterVersion(stmp);

				m_sessVersion = NO_SEQNUM;
				bProcessed = TRUE;
			}
			break;

		case SYSTEM_PACKET_MEMORY_STAT_REPLY:
			if ( m_sessMemStat == sessId )  {
				union systemPacketArgs*	args;

				args = (union systemPacketArgs*)pContainer->getPacketHead();
				m_memStat = args->memory_stat_reply.stat;
				updateMemoryStat();

				m_sessMemStat = NO_SEQNUM;
				bProcessed = TRUE;
			}
			break;

		case SYSTEM_PACKET_NETCONN_STAT_REPLY:
			if ( m_sessNetConnStat == sessId )  {
				union systemPacketArgs*	args;

				args = (union systemPacketArgs*)pContainer->getPacketHead();
				m_netconnStat = args->netconn_stat_reply.stat;
				updateNetConnStat();

				m_sessNetConnStat = NO_SEQNUM;
				bProcessed = TRUE;
			}
			break;

		case SYSTEM_PACKET_LOGGER_CHANNEL:
			if ( m_sessLogCtl == sessId )  {
				union systemPacketArgs*	args;

				args = (union systemPacketArgs*)pContainer->getPacketHead();
				updateLogControl(args->logger_channel.channel);

				m_sessLogCtl = NO_SEQNUM;
				bProcessed = TRUE;
			}
			break;

		case SYSTEM_PACKET_RESULT:
			if ( m_sessLogEnable == sessId )  {
				union systemPacketArgs*	args;

				args = (union systemPacketArgs*)pContainer->getPacketHead();
				if ( args->result.nresult != ESUCCESS )  {
					log_debug(L_GEN, "[accapp] external logger channel enable returns error: %d\n",
							  args->result.nresult);
				}

				m_sessLogEnable = NO_SEQNUM;
				bProcessed = TRUE;
			}
			break;
	}

	return bProcessed;
}

void CAccApp::processIOResult(seqnum_t sessId, result_t nresult)
{
	if ( sessId == m_sessHeartBeat )  {
		if ( m_bCenterConnected && nresult != ESUCCESS ) {
			m_bCenterConnected = FALSE;
			m_pInterface->updateNetState(FALSE);
			m_pInterface->updateCenterVersion("is not connected");
		}
		m_sessHeartBeat = NO_SEQNUM;
	} else
	if ( sessId == m_sessVersion )  {
		m_sessVersion = NO_SEQNUM;
	} else
	if ( sessId == m_sessHostInsert )  {
		m_sessHostInsert = NO_SEQNUM;
		m_pInterface->updateBusyState(FALSE);
	} else
	if ( sessId == m_sessHostUpdate )  {
		m_sessHostUpdate = NO_SEQNUM;
		m_pInterface->updateBusyState(FALSE);
	} else
	if ( sessId == m_sessHostDelete )  {
		m_sessHostDelete = NO_SEQNUM;
		m_pInterface->updateBusyState(FALSE);
	} else
	if ( sessId == m_sessMemStat )  {
		m_sessMemStat = NO_SEQNUM;
	} else
	if ( sessId == m_sessNetConnStat )  {
		m_sessNetConnStat = NO_SEQNUM;
	} else
	if ( sessId == m_sessLogCtl )  {
		m_sessLogCtl = NO_SEQNUM;
	} else
	if ( sessId == m_sessLogEnable )  {
		m_sessLogEnable = NO_SEQNUM;
	}
}

boolean_t CAccApp::processPacket(CEventTcpConnRecv* pEventRecv)
{
	CVepContainer*	pContainer;
	boolean_t		bProcessed = FALSE;

	shell_assert(pEventRecv);

	pContainer = dynamic_cast<CVepContainer*>(pEventRecv->getContainer());
	if ( !pContainer )  {
		return FALSE;
	}

	switch ( pContainer->getType() )  {
		case VEP_CONTAINER_SYSTEM:
			bProcessed = processPacketSystem(pEventRecv);
			break;

		case VEP_CONTAINER_CENTER:
			bProcessed = processPacketCenter(pEventRecv);
			break;

		default:
			log_debug(L_GEN, "[accapp] unknown container type: %u dropped\n",
				pContainer->getType());
			break;
	}

	return bProcessed;
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
boolean_t CAccApp::processEvent(CEvent* pEvent)
{
	CEventTcpConnRecv*		pEventRecv;
	CEventInsertHost*		pEventInsertHost;
	CEventUpdateHost*		pEventUpdateHost;
	boolean_t				bProcessed = FALSE;

	if ( CApplication::processEvent(pEvent) )  {
		return TRUE;
	}

	switch ( pEvent->getType() ) {
		case EV_NETCONN_RECV:
			pEventRecv = dynamic_cast<CEventTcpConnRecv*>(pEvent);
			shell_assert(pEventRecv);
			if ( pEventRecv ) {
				bProcessed = processPacket(pEventRecv);
			}
			break;

		case EV_NETCONN_SENT:
			processIOResult(pEvent->getSessId(), (result_t)pEvent->getnParam());
			bProcessed = TRUE;
			break;

		case EV_HB_ENABLE:
			switchHeartBeat((boolean_t)pEvent->getnParam());
			bProcessed = TRUE;
			break;

		case EV_HOST_INSERT:
			pEventInsertHost = dynamic_cast<CEventInsertHost*>(pEvent);
			shell_assert(pEventInsertHost);
			requestHostInsert(pEventInsertHost->getData()->strHost,
						  pEventInsertHost->getData()->ipHost);
			bProcessed = TRUE;
			break;

		case EV_HOST_UPDATE:
			pEventUpdateHost = dynamic_cast<CEventUpdateHost*>(pEvent);
			shell_assert(pEventUpdateHost);
			requestHostUpdate(pEventUpdateHost->getData()->id,
							  pEventUpdateHost->getData()->strHost,
							  pEventUpdateHost->getData()->ipHost);
			bProcessed = TRUE;
			break;

		case EV_HOST_REMOVE:
			requestHostDelete((host_id_t)pEvent->getnParam());
			bProcessed = TRUE;
			break;

		case EV_MEMORY_STAT:
			doMemoryStat((int)pEvent->getnParam(), (ip_addr_t)(long int)pEvent->getpParam());
			bProcessed = TRUE;
			break;

		case EV_NETCONN_STAT:
			doNetConnStat((int)pEvent->getnParam(), (ip_addr_t)(long int)pEvent->getpParam());
			bProcessed = TRUE;
			break;

		case EV_LOG_CONTROL:
			doLogControl((int)pEvent->getnParam(), (ip_addr_t)(long int)pEvent->getpParam());
			bProcessed = TRUE;
			break;

		case EV_LOG_CONTROL_CHECK:
			doLogControlCheck((unsigned int)(size_t)pEvent->getpParam(), (boolean_t)pEvent->getnParam());
			bProcessed = TRUE;
			break;
	}

	return bProcessed;
}

void CAccApp::stopUpdateTimer()
{
	SAFE_DELETE_TIMER(m_pUpdateTimer, getMainLoop());
}

void CAccApp::updateHandler(void* p)
{
	requestData();
}

void CAccApp::startUpdateTimer()
{
	shell_assert(m_pUpdateTimer == 0);

	m_pUpdateTimer = new CTimer(ACC_UPDATE_PERIOD, TIMER_CALLBACK(CAccApp::updateHandler, this),
								CTimer::timerPeriodic, "update-timer");
	getMainLoop()->insertTimer(m_pUpdateTimer);
}

void CAccApp::stopMemoryStatTimer()
{
	SAFE_DELETE_TIMER(m_pMemStatTimer, getMainLoop());
}

void CAccApp::restartMemoryStatTimer()
{
	if ( m_pMemStatTimer )  {
		m_pMemStatTimer->restart();
	}
	else {
		m_pMemStatTimer = new CTimer(MEMORY_STAT_INTERVAL,
			TIMER_CALLBACK(CAccApp::memoryStatHandler, this),
			CTimer::timerPeriodic, "memory-stat");
		getMainLoop()->insertTimer(m_pMemStatTimer);
	}
}

void CAccApp::memoryStatHandler(void* p)
{
	switch ( m_nMemStatType )  {
		case MEMORY_STAT_LOCAL:
			getLocalMemoryStat();
			updateMemoryStat();
			break;

		case MEMORY_STAT_CENTER:
		case MEMORY_STAT_EXTERNAL:
			if ( m_sessMemStat == NO_SEQNUM )  {
				getExternalMemoryStat();
			}
			break;

		default:
			shell_assert(FALSE);
	}
}

/*
 * Network Connector Statistic timer
 */
void CAccApp::stopNetConnStatTimer()
{
	SAFE_DELETE_TIMER(m_pNetConnStatTimer, getMainLoop());
}

void CAccApp::restartNetConnStatTimer()
{
	if ( m_pNetConnStatTimer )  {
		m_pNetConnStatTimer->restart();
	}
	else {
		m_pNetConnStatTimer = new CTimer(NETCONN_STAT_INTERVAL,
									 TIMER_CALLBACK(CAccApp::netconnStatHandler, this),
									 CTimer::timerPeriodic, "netconn-stat");
		getMainLoop()->insertTimer(m_pNetConnStatTimer);
	}
}

void CAccApp::netconnStatHandler(void* p)
{
	switch ( m_nNetConnStatType )  {
		case NETCONN_STAT_LOCAL:
			getLocalNetConnStat();
			updateNetConnStat();
			break;

		case NETCONN_STAT_CENTER:
		case NETCONN_STAT_EXTERNAL:
			if ( m_sessNetConnStat == NO_SEQNUM )  {
				getExternalNetConnStat();
			}
			break;

		default:
			shell_assert(FALSE);
	}
}


void CAccApp::initLogger()
{
	CApplication::initLogger();
	//logger_enable(LT_DEBUG|L_NET_FL);
	//logger_enable(LT_ALL|L_EV_TRACE_EVENT);
	//logger_enable(LT_ALL|L_NETCONN_PACK);
}

result_t CAccApp::init()
{
	char		strTmp[128];
	result_t	nresult;

	nresult = CApplication::init();
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	copyString(strTmp, getAppPath(), sizeof(strTmp));
	appendPath(strTmp, "acc.conf", sizeof(strTmp));
	nresult = m_config.load(strTmp, TRUE);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[accapp] failed to load config %s, result: %d\n", strTmp, nresult);
	}

	m_addrCenter = CNetAddr("127.0.0.1", 10001);
	initCenterPacket();         /* Initialise packet string table */
	CEventStringTable::registerStringTable(EV_HB_ENABLE, EV_LOG_CONTROL_CHECK, accEventTable);

	shell_assert(m_pNetConnector == 0);
	shell_assert(m_pInterface == 0);

	m_pNetConnector = new CTcpConnector(new CVepContainer, this, 10);
	m_pInterface = new CAccInterface(this);

	CModule* arModules[] = { m_pNetConnector, m_pInterface };

	nresult = appInitModules(arModules, ARRAY_SIZE(arModules));
	if ( nresult == ESUCCESS ) {
		startUpdateTimer();	/* Start periodic timer */
	}
	else {
		CApplication::terminate();
	}

	return nresult;
}

void CAccApp::terminate()
{
	CModule* arModules[] = { m_pNetConnector, m_pInterface };

	m_config.update();

	stopUpdateTimer();
	stopMemoryStatTimer();
	stopNetConnStatTimer();

	appTerminateModules(arModules, ARRAY_SIZE(arModules));
	SAFE_DELETE(m_pInterface);
	SAFE_DELETE(m_pNetConnector);

	CApplication::terminate();
}

int main(int argc, char **argv)
{
	CAccApp	theApp(argc, argv);

	return theApp.run();
}
