/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTCP protocol client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 08.11.2016 13:01:07
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTCP_CLIENT_H_INCLUDED__
#define __NET_MEDIA_RTCP_CLIENT_H_INCLUDED__

#include "shell/counter.h"

#include "carbon/timer.h"
#include "carbon/net_connector/udp_connector.h"

#include "net_media/rtp_session.h"
#include "net_media/rtcp.h"

class CRtcpClient : public CModule, public CEventReceiver
{
	protected:
		CEventLoop*			m_pOwnerLoop;
		CNetHost			m_selfHost;			/* Self host ip */
		CNetHost			m_serverHost;		/* Server host ip */
		ip_port_t			m_nRtcpPort;		/* RTCP communication port (unicast) */
		ip_port_t			m_nRtcpServerPort;	/* RTCP server side communication port */

		CUdpConnector		m_netConn;			/* Network I/O connector */

		uint32_t			m_nServerSsrc;		/* Server synchronisation source */
		uint32_t			m_nSelfSsrc;		/* Self synchronisation source */

		CTimer*				m_pRrTimer;
		CRtpSessionManager*	m_pSessionMan;

		counter_t			m_nPackets;			/* Received the valid packets */
		counter_t			m_nInvalidPackets;	/* Received the invalid packets */
		counter_t			m_nSrPackets;		/* Received SR packets from our source */
		counter_t			m_nRrSentPackets;	/* Sent RR packets */

	public:
		CRtcpClient(const CNetHost& selfHost, CEventLoop* pOwnerLoop, CRtpSessionManager* pSessionMan);
		virtual ~CRtcpClient();

	public:
		virtual result_t init2(const CNetHost& serverHost, ip_port_t nRtcpPort,
							  ip_port_t nRtcpServerPort, uint32_t nServerSsrc);
		virtual void terminate();

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);

		void startReceive();
		void formatPacketHead(rtcp_head_t* pHead, boolean_t bPadding, int itemCount,
							  int type, int length) const;
		size_t formatSdesPacket(void* pData) const;
		size_t formatRrPacket(void* pData);

		void ntohReportBlock(rtcp_report_block_t* pReportBlock) const;
		void htonReportBlock(rtcp_report_block_t* pReportBlock) const;

		result_t validatePacket(void* pData, size_t* pLength) const;
		void processPacket(void* pData, size_t length);
		void processSrPacket(rtcp_sr_packet_t* pPacket);

		void stopRrTimer();
		void startRtTimer();
		void rrTimerHandler(void* p);
};

#endif /* __NET_MEDIA_RTCP_CLIENT_H_INCLUDED__ */
