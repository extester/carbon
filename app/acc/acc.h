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
 *  Revision 1.0, 14.07.2015 16:11:59
 *      Initial revision.
 */

#ifndef __ACC_H_INCLUDED__
#define __ACC_H_INCLUDED__


#include "carbon/carbon.h"
#include "carbon/event.h"
#include "carbon/timer.h"
#include "carbon/net_connector/tcp_connector.h"
#include "carbon/application.h"
#include "carbon/config_xml.h"

#include "memory_stat.h"
#include "netconn_stat.h"
#include "log_control.h"
#include "../center/packet_center.h"
#include "interface.h"

#define ACC_NAME					"Carbon Center Client"
#define ACC_VERSION					MAKE_VERSION(0,1)

#define EV_HB_ENABLE				(EV_USER+0)
#define EV_HOST_INSERT				(EV_USER+1)
#define EV_HOST_UPDATE				(EV_USER+2)
#define EV_HOST_REMOVE				(EV_USER+3)
#define EV_MEMORY_STAT				(EV_USER+4)
#define EV_NETCONN_STAT				(EV_USER+5)
#define EV_LOG_CONTROL				(EV_USER+6)
#define EV_LOG_CONTROL_CHECK		(EV_USER+7)

typedef struct
{
	char		strHost[HOSTLIST_NAME_MAX];
	CNetHost	ipHost;
} insert_host_data_t;

typedef struct
{
	host_id_t	id;
	char		strHost[HOSTLIST_NAME_MAX];
	CNetHost	ipHost;
} update_host_data_t;

typedef	CEventT<insert_host_data_t, EV_HOST_INSERT>		CEventInsertHost;
typedef	CEventT<update_host_data_t, EV_HOST_UPDATE>		CEventUpdateHost;


class CAccApp : public CApplication
{
	private:
		CNetAddr			m_addrCenter;
		CTcpConnector*		m_pNetConnector;
		CAccInterface*		m_pInterface;
		CConfig				m_config;

		CTimer*				m_pUpdateTimer;
		boolean_t			m_bCenterConnected;

		seqnum_t			m_sessVersion;
		seqnum_t			m_sessHeartBeat;
		seqnum_t			m_sessHostInsert;
		seqnum_t			m_sessHostUpdate;
		seqnum_t			m_sessHostDelete;
		seqnum_t			m_sessMemStat;
		seqnum_t			m_sessNetConnStat;
		seqnum_t			m_sessLogCtl;
		seqnum_t			m_sessLogEnable;

		int 				m_nMemStatType;
		CNetAddr			m_memStatExtAddr;
		memory_stat_t		m_memStat;
		CTimer*				m_pMemStatTimer;

		int					m_nNetConnStatType;
		CNetAddr			m_netconnStatExtAddr;
		tcpconn_stat_t		m_netconnStat;
		CTimer*				m_pNetConnStatTimer;

		int 				m_nLogCtl;
		CNetAddr			m_logctlExtAddr;

	public:
		CAccApp(int argc, char* argv[]);
		virtual ~CAccApp();

		CConfig* getConfig() {
			return &m_config;
		}

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);
		virtual void initLogger();
		virtual result_t init();
		virtual void terminate();

	private:
		boolean_t processPacketCenter(CEventTcpConnRecv* pEvenRecv);
		boolean_t processPacketSystem(CEventTcpConnRecv* pEventRecv);
		void processIOResult(seqnum_t sessId, result_t nresult);

		boolean_t processPacket(CEventTcpConnRecv* pEventRecv);

		void switchHeartBeat(boolean_t bEnable);
		result_t doRequest(CVepContainer* pContainer, seqnum_t* pSessId);
		void requestData();
		void requestCenterVersion();
		void requestHostInsert(const char* strHost, const CNetHost& ipHost);
		void requestHostUpdate(host_id_t id, const char* strHost, const CNetHost& ipHost);
		void requestHostDelete(host_id_t id);

		void getLocalMemoryStat();
		void getExternalMemoryStat();
		void updateMemoryStat();
		void doMemoryStat(int type, ip_addr_t extAddr);

		void getLocalNetConnStat();
		void getExternalNetConnStat();
		void updateNetConnStat();
		void doNetConnStat(int type, ip_addr_t extAddr);

		void getLocalLogControl(uint8_t* pChannel);
		void getExternalLogControl();
		void updateLogControl(const uint8_t* pChannel);
		void doLogControl(int type, ip_addr_t extAddr);
		void doLogExternalEnable(unsigned int nChannel, boolean_t isOn);
		void doLogControlCheck(unsigned int nChannel, boolean_t isOn);

		void stopUpdateTimer();
		void startUpdateTimer();
		void updateHandler(void* p);

		void stopMemoryStatTimer();
		void restartMemoryStatTimer();
		void memoryStatHandler(void* p);

		void stopNetConnStatTimer();
		void restartNetConnStatTimer();
		void netconnStatHandler(void* p);
};

#endif /* __ACC_H_INCLUDED__ */
