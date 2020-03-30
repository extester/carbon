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
 *	Revision 1.0, 08.11.2016 13:10:03
 *		Initial revision.
 */

#include "shell/utils.h"
#include "carbon/raw_container.h"
#include "carbon/event/eventloop.h"
#include "net_media/rtcp_client.h"

#define MODULE_NAME				"rtcp_client"
#define RTCP_PACKET_SIZE_MAX	4096

/*******************************************************************************
 * CRtcpClient class
 */

CRtcpClient::CRtcpClient(const CNetHost& selfHost, CEventLoop* pOwnerLoop,
						 CRtpSessionManager* pSessionMan) :
	CModule(MODULE_NAME),
	CEventReceiver(pOwnerLoop, MODULE_NAME),
	m_pOwnerLoop(pOwnerLoop),
	m_selfHost(selfHost),
	m_nRtcpPort(0),
	m_nRtcpServerPort(0),
	m_netConn(new CRawContainer(RTCP_PACKET_SIZE_MAX), this),
	m_nServerSsrc(0),
	m_pRrTimer(0),
	m_pSessionMan(pSessionMan),

	m_nPackets(ZERO_COUNTER),
	m_nInvalidPackets(ZERO_COUNTER),
	m_nSrPackets(ZERO_COUNTER),
	m_nRrSentPackets(ZERO_COUNTER)
{
	/* Generate self SSRC */
	do {
		m_nSelfSsrc = (unsigned int)(random()%(0xffffffff));
	} while ( m_nSelfSsrc == 0 );
}

CRtcpClient::~CRtcpClient()
{
}

/*
 * Convert receiver report block data order from network to host
 *
 * 		pReportBlock 	report block in network byteorder
 */
void CRtcpClient::ntohReportBlock(rtcp_report_block_t* pReportBlock) const
{
	pReportBlock->reporteeSsrc = ntohl(pReportBlock->reporteeSsrc);
	pReportBlock->lost = ntohl(pReportBlock->lost);
	pReportBlock->highSequence = ntohl(pReportBlock->highSequence);
	pReportBlock->jitter = ntohl(pReportBlock->jitter);
	pReportBlock->lsr = ntohl(pReportBlock->lsr);
	pReportBlock->dlsr = ntohl(pReportBlock->dlsr);
}

/*
 * Convert receiver report block data order from host to network
 *
 * 		pReportBlock 	report block in host byteorder
 */
void CRtcpClient::htonReportBlock(rtcp_report_block_t* pReportBlock) const
{
	pReportBlock->reporteeSsrc = htonl(pReportBlock->reporteeSsrc);
	pReportBlock->lost = htonl(pReportBlock->lost);
	pReportBlock->highSequence = htonl(pReportBlock->highSequence);
	pReportBlock->jitter = htonl(pReportBlock->jitter);
	pReportBlock->lsr = htonl(pReportBlock->lsr);
	pReportBlock->dlsr = htonl(pReportBlock->dlsr);
}

/*
 * Validate, calculate real packet length (exclude padding) and
 * convert RTCP packet header from network byte order to host byte order
 *
 * 		pData			packet data
 * 		length			packet data length [in/out]
 *
 * Return:
 * 		ESUCCESS	converted successful
 * 		EINVAL		packet is not valid
 */
result_t CRtcpClient::validatePacket(void* pData, size_t* pLength) const
{
	union rtcp_packet* 		pPacket = 0;
	rtcp_sr_packet_t*		pSrPacket;
	rtcp_rr_packet_t*		pRrPacket;
	rtcp_sdes_data_t*		pSdesData;
	rtcp_bye_packet_t*		pByePacket;
	rtcp_app_packet_t*		pAppPacket;
	char					*s, sdes_type, sdes_len;

	uint32_t				hh;
	size_t					ic, len, l, i, compoundCount = 0, compoundLength = 0;
	uint8_t					*p, *end, *next;
	const size_t			length = *pLength;
	result_t				nresult = ESUCCESS;

	p = (uint8_t*)pData;
	end = p+length;

	if ( length < sizeof(rtcp_head_t) )  {
		log_trace(L_RTCP, "[rtcp_cli] compound packet too short (%d)!\n", length);
		return EINVAL;
	}

	while ( nresult == ESUCCESS && p < end )  {
		pPacket = (union rtcp_packet*)p;

		pPacket->h.head = ntohl(pPacket->h.head);
		next = p + sizeof(rtcp_head_t) + RTCP_HEAD_LENGTH(pPacket->h.head)*sizeof(uint32_t);
		if ( next > end )  {
			log_trace(L_RTCP, "[rtcp_cli] i=%u: compound packet too short1!\n", compoundCount);
			nresult = EINVAL;
			break;
		}

		hh = pPacket->h.head;
		if ( RTCP_HEAD_VERSION(hh) != RTCP_VERSION )  {
			log_trace(L_RTCP, "[rtcp_cli] i=%u: invalid packet version %d, extected %d\n",
					  compoundCount, RTCP_HEAD_VERSION(hh), RTCP_VERSION);
			nresult = EINVAL;
			break;
		}

		ic = RTCP_HEAD_ITEM_COUNT(hh);
		len = RTCP_HEAD_LENGTH(hh)*sizeof(uint32_t);
		compoundLength += sizeof(rtcp_head_t)+len;

		switch ( RTCP_HEAD_PACKET_TYPE(hh) )  {
			case RTCP_PACKET_SR:
				l = sizeof(rtcp_sr_packet_t)-sizeof(rtcp_head_t)+ic*sizeof(rtcp_report_block_t);
				if ( l > len )  {
					log_trace(L_RTCP, "[rtcp_cli] i=%u: sr packet length too short\n", compoundCount);
					nresult = EINVAL;
					break;
				}

				pSrPacket = &pPacket->sr;
				pSrPacket->ssrc = ntohl(pSrPacket->ssrc);
				pSrPacket->ntp_timestamp = ntohll(pSrPacket->ntp_timestamp);
				pSrPacket->rtp_timestamp = ntohl(pSrPacket->rtp_timestamp);
				pSrPacket->packet_count = ntohl(pSrPacket->packet_count);
				pSrPacket->octet_count = ntohl(pSrPacket->octet_count);

				for(i=0; i<ic; i++)  {
					ntohReportBlock(&pSrPacket->rb[i]);
				}
				break;

			case RTCP_PACKET_RR:
				l = sizeof(rtcp_rr_packet_t)-sizeof(rtcp_head_t)+ic*sizeof(rtcp_report_block_t);
				if ( l > len )  {
					log_trace(L_RTCP, "[rtcp_cli] i=%d: rr packet length too short (l=%u, len=%u)\n",
							  compoundCount, l, len);
					nresult = EINVAL;
					break;
				}

				pRrPacket = &pPacket->rr;
				pRrPacket->ssrc = ntohl(pRrPacket->ssrc);

				for(i=0; i<ic; i++)  {
					ntohReportBlock(&pRrPacket->rb[i]);
				}
				break;

			case RTCP_PACKET_SDES:
				l = 0;
				for(i=0; i<ic; i++)  {
					l = ALIGN(l, sizeof(uint32_t));
					pSdesData = (rtcp_sdes_data_t*)(p+sizeof(rtcp_head_t)+l);

					if ( (l+sizeof(pSdesData->ssrc)) >= len ) {
						log_trace(L_RTCP, "[rtcp_cli] i=%d: sdes packet too short1\n", compoundCount);
						nresult = EINVAL;
						break;
					}
					l += sizeof(pSdesData->ssrc);

					pSdesData->ssrc = ntohl(pSdesData->ssrc);
					s = pSdesData->list;

					while ( l < len )  {
						if ( (l+1) > len ) {
							log_trace(L_RTCP, "[rtcp_cli] i=%d: sdes packet too short2\n", compoundCount);
							nresult = EINVAL;
							break;
						}
						l += 1;

						sdes_type = *s;
						if ( sdes_type == 0 ) {
							break;
						}

						if ( (l+1) > len ) {
							log_trace(L_RTCP, "[rtcp_cli] i=%d: sdes packet too short3\n", compoundCount);
							nresult = EINVAL;
							break;
						}
						l += 1;

						sdes_len = *(s+1);

						if ( (l+sdes_len) > len )  {
							log_trace(L_RTCP, "[rtcp_cli] i=%d: sdes packet too short4\n", compoundCount);
							nresult = EINVAL;
							break;
						}
						l += sdes_len;

						s += 2+sdes_len;
					}

					if ( nresult != ESUCCESS )  {
						break;
					}
				}
				break;

			case RTCP_PACKET_BYE:
				l = sizeof(rtcp_bye_packet_t)-sizeof(rtcp_head_t)+ic*sizeof(uint32_t);
				if ( l > len )  {
					log_trace(L_RTCP, "[rtcp_cli] i=%d: bye packet length too short\n", compoundCount);
					nresult = EINVAL;
					break;
				}

				pByePacket = &pPacket->bye;
				for(i=0; i<ic; i++)  {
					pByePacket->ssrc[i] = ntohl(pByePacket->ssrc[i]);
				}

				if ( (l+1) < len )  {
					/* There is an optional text (reason for leaving) */
					s = (char*)p+l;

					if ( l+1+(*s) > len )  {
						log_trace(L_RTCP, "[rtcp_cli] i=%d: bye packet reason length too short\n",
								  compoundCount);
						nresult = EINVAL;
						break;
					}
				}

				break;

			case RTCP_PACKET_APP:
				l = sizeof(rtcp_app_packet_t)-sizeof(rtcp_head_t);
				if ( l > len )  {
					log_trace(L_RTCP, "[rtcp_cli] i=%d: app packet length too short\n", compoundCount);
					nresult = EINVAL;
					break;
				}

				pAppPacket = &pPacket->app;
				pAppPacket->ssrc = ntohl(pAppPacket->ssrc);
				break;

			default:
				log_trace(L_RTCP, "[rtcp_cli] i=%d: unknown packet type %d\n",
						  compoundCount, RTCP_HEAD_PACKET_TYPE(hh));
				nresult = EINVAL;
		}

		p = next;
		compoundCount++;
	}

	/*
	 * Check for padding
	 * pPacket - last packet pointer
	 */
	if ( nresult == ESUCCESS && pPacket != 0 && RTCP_HEAD_PADDING(pPacket->h.head) )  {
		uint8_t*	pPadCount = ((uint8_t*)pData) + length - sizeof(uint8_t);

		if ( length >= (compoundLength+(*pPadCount)) ) {
			*pLength = length-(*pPadCount);
		}
		else {
			log_trace(L_RTCP, "[rtcp_cli] padding wrong (%d bytes)\n", *pPadCount);
			nresult = EINVAL;
		}
	}

	return nresult;
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
boolean_t CRtcpClient::processEvent(CEvent* pEvent)
{
	CEventUdpRecv*		pEventRecv;
	CRawContainer*		pContainer;
	boolean_t			bProcessed = FALSE;


	switch ( pEvent->getType() )  {
		case EV_NETCONN_RECV:
			pEventRecv = dynamic_cast<CEventUdpRecv*>(pEvent);
			shell_assert(pEventRecv);
			if ( pEventRecv )  {
				pContainer = dynamic_cast<CRawContainer*>(pEventRecv->getContainer());
				shell_assert(pContainer);
				if ( pContainer )  {
					processPacket(pContainer->getData(), pContainer->getSize());
				}
			}

			bProcessed = TRUE;
			break;
	}

	return bProcessed;
}

void CRtcpClient::processPacket(void* pData, size_t length)
{
	union rtcp_packet	*pPacket;
	void*				pEnd;
	size_t				rlength;
	int 				type;
	result_t			nresult;

	log_trace(L_RTCP, "[rtcp_cli] RECEIVED A PACKET (%d bytes)\n", (int)length);

	rlength = length;
	nresult = validatePacket(pData, &rlength);
	if ( nresult != ESUCCESS )  {
		log_debug(L_RTCP, "[rtcp_cli] invalid packet received\n");
		counter_inc(m_nInvalidPackets);
		return;
	}

	counter_inc(m_nPackets);
	//dumpRtcpCompoundPacket(pData, rlength);

	pPacket = (union rtcp_packet*)pData;
	pEnd = ((uint8_t*)pData) + rlength;

	while ( pPacket < pEnd ) {
		type = RTCP_HEAD_PACKET_TYPE(pPacket->h.head);

		switch (type) {
			case RTCP_PACKET_SR:
				processSrPacket(&pPacket->sr);
				counter_inc(m_nSrPackets);
				break;

			case RTCP_PACKET_RR:
			case RTCP_PACKET_SDES:
			case RTCP_PACKET_BYE:
			case RTCP_PACKET_APP:
				break;

			default:
				log_trace(L_RTCP, "[rtcp_cli] received an unknown RTCP packet type %d\n", type);
		}

		pPacket = (union rtcp_packet*)(((uint8_t*)pPacket)+(RTCP_HEAD_LENGTH(pPacket->h.head)+1)*sizeof(uint32_t));
	}
}

void CRtcpClient::processSrPacket(rtcp_sr_packet_t* pPacket)
{
	if ( pPacket->ssrc == m_nServerSsrc )  {
		/* This is out interested packet */
		shell_assert(m_pSessionMan);

		m_pSessionMan->onRtcpSrPacket(pPacket);
	}
}


/*
 * Start receiving the RTCP packets
 */
void CRtcpClient::startReceive()
{
	CNetAddr	listenAddr = CNetAddr(m_selfHost, m_nRtcpPort);
	int 		n = 5;
	result_t	nresult = ESUCCESS;

	while ( n > 0 )  {
		nresult = m_netConn.startListen(listenAddr);
		if ( nresult == ESUCCESS || nresult == ECANCELED )  {
			break;
		}
		sleep_s(1);
		n--;
	}

	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[rtcp_cli] CAN'T RECEIVE RTCP PACKETS, result %d\n", nresult);
	}
}

/*
 * Format a RTCP output packet header
 *
 * 		pHead			packet header [out]
 * 		bPadding		padding
 * 		itemCount		item count field
 * 		type			RTCP packet type, RTCP_PACKET_XX
 * 		length			packet payload length in dwords
 *
 * Note: output head in network byte order
 */
void CRtcpClient::formatPacketHead(rtcp_head_t* pHead, boolean_t bPadding,
								   int itemCount, int type, int length) const
{
	rtcp_head_t		h;

	shell_assert(itemCount <= 0x1f);
	shell_assert(type <= 0xff);
	shell_assert(length <= 0xffff);

	h.head = (RTCP_VERSION << 30) |
			((uint32_t)bPadding << 29) |
			((itemCount&0x1f) << 24) |
			(type << 16) |
			(length&0xffff);

	pHead->head = htonl(h.head);
}

/*
 * Format a SDES RTCP packet
 *
 * 		pData		output buffer of RTCP_PACKET_SIZE_MAX length
 *
 * Return: full packet length in bytes
 */
size_t CRtcpClient::formatSdesPacket(void* pData) const
{
	rtcp_sdes_packet_t*		pPacket = (rtcp_sdes_packet_t*)pData;
	rtcp_sdes_data_t*		pSdesData = pPacket->sdes;
	char 					strBuf[128];
	int 					cnameLen, sdesLen;

	shell_assert(m_nSelfSsrc);

	cnameLen = _tsnprintf(strBuf, sizeof(strBuf), "%X@%s", m_nSelfSsrc, (const char*)m_selfHost);

	pSdesData->ssrc = htonl(m_nSelfSsrc);
	pSdesData->list[0] = RTCP_SDES_ITEM_CNAME;
	pSdesData->list[1] = (char)cnameLen;
	UNALIGNED_MEMCPY(&pSdesData->list[2], strBuf, cnameLen+1);
	pSdesData->list[2+cnameLen] = 0;

	sdesLen = 2 + cnameLen + 1;
	_tmemset(&pSdesData->list[sdesLen], 0, ALIGN(sdesLen, sizeof(uint32_t)-sdesLen));

	sdesLen = sizeof(rtcp_sdes_data_t) + ALIGN(sdesLen, sizeof(uint32_t));

	formatPacketHead(&pPacket->h, FALSE/*no padding*/, 1/*IC*/, RTCP_PACKET_SDES,
						sdesLen/sizeof(uint32_t));

	return sizeof(rtcp_head_t) + sdesLen;
}

/*
 * Format a RR RTCP packet
 *
 * 		pData		output buffer of RTCP_PACKET_SIZE_MAX length
 *
 * Return: full packet length in bytes
 */
size_t CRtcpClient::formatRrPacket(void* pData)
{
	rtcp_rr_packet_t*	pPacket = (rtcp_rr_packet_t*)pData;

	shell_assert(m_pSessionMan);

	pPacket->ssrc = htonl(m_nSelfSsrc);

	m_pSessionMan->getSessionData(m_nServerSsrc, &pPacket->rb[0]);
	pPacket->rb[0].reporteeSsrc = m_nServerSsrc;
	htonReportBlock(&pPacket->rb[0]);

	formatPacketHead(&pPacket->h, FALSE/*no padding*/, 1/*IC*/, RTCP_PACKET_RR,
					 1+sizeof(rtcp_report_block_t)/sizeof(uint32_t));

	return sizeof(rtcp_rr_packet_t)+sizeof(rtcp_report_block_t);
}

#define RTCP_RR_TIMER_MIN		8000
#define RTCP_RR_TIMER_MAX		16000

void CRtcpClient::stopRrTimer()
{
	SAFE_DELETE_TIMER(m_pRrTimer, m_pOwnerLoop);
}

void CRtcpClient::startRtTimer()
{
	int 	msTimeout;

	stopRrTimer();

	msTimeout = RTCP_RR_TIMER_MIN + (int)(random()%(RTCP_RR_TIMER_MAX-RTCP_RR_TIMER_MIN));

	m_pRrTimer = new CTimer(MILLISECONDS_TO_HR_TIME(msTimeout),
							TIMER_CALLBACK(CRtcpClient::rrTimerHandler, this),
							0, "rr");
	m_pOwnerLoop->insertTimer(m_pRrTimer);
}

void CRtcpClient::rrTimerHandler(void* p)
{
	dec_ptr<CRawContainer>	pContainer = new CRawContainer(RTCP_PACKET_SIZE_MAX);
	uint8_t		buf[RTCP_PACKET_SIZE_MAX];
	size_t		length;
	result_t	nresult;

	m_pRrTimer = 0;

	length = formatRrPacket(buf);
	length += formatSdesPacket(&buf[length]);

	nresult = pContainer->putData(buf, length);
	if ( nresult == ESUCCESS )  {
		CNetAddr	servAddr(m_serverHost, m_nRtcpServerPort);
//{ size_t len = length;
//	shell_assert(validatePacket(buf, &len) == 0);
//dumpRtcpCompoundPacket(buf, length);}

		log_trace(L_RTCP, "[rtcp_cli] sending RTCP packet to %s..\n", (const char*)servAddr);
		nresult = m_netConn.sendSync(pContainer, servAddr);
		if ( nresult == ESUCCESS ) {
			counter_inc(m_nRrSentPackets);
		}
		else {
			log_error(L_RTCP, "[rtcp_cli] failed to send RTCP packet, result %d\n", nresult);
		}
	}
	else {
		log_error(L_RTCP, "[rtcp_cli] failed to send RTCP packet, put failed, result %d\n", nresult);
	}

	startRtTimer();
}

/*
 * Initialise RTCP client
 *
 * 		serverHost			RTCP server ip
 * 		nRtcpPort			client (local) RTCP port (bind to)
 * 		nRtcpServerPort		server RTCP port (send RTCP packets to)
 * 		nServerSsrc			RTCP server ident
 *
 * Return: ESUCCESS, ...
 */
result_t CRtcpClient::init2(const CNetHost& serverHost, ip_port_t nRtcpPort,
						   ip_port_t nRtcpServerPort, uint32_t nServerSsrc)
{
	CNetAddr	bindAddr;
	result_t	nresult;

	shell_assert(nRtcpPort);
	shell_assert(nRtcpServerPort);
	shell_assert(nServerSsrc);
return ESUCCESS; /* UNCOMPLETED */
	m_nRtcpPort = nRtcpPort;
	m_nRtcpServerPort = nRtcpServerPort;
	m_nServerSsrc = nServerSsrc;
	m_serverHost = serverHost;

	nresult = CModule::init();
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	bindAddr = CNetAddr(m_selfHost, m_nRtcpPort);
	//log_debug(L_GEN, "=================> INIT BIND TO %s\n", (const char*)bindAddr);
	m_netConn.setBindAddr(bindAddr);

	nresult = m_netConn.init();
	if ( nresult != ESUCCESS )  {
		CModule::terminate();
		return nresult;
	}

	startReceive();
	startRtTimer();

	return ESUCCESS;
}

/*
 * Cancel any operations and stop RTCP client
 */
void CRtcpClient::terminate()
{
	stopRrTimer();
	m_netConn.terminate();
	CModule::terminate();
}

/*******************************************************************************
 * Debugging support
 */

void CRtcpClient::dump(const char* strPref) const
{
	log_dump("*** %sRTCP client: selfHost: %s, serverHost: %s, rtcpPort: %u, rtcpServerPort: %u\n",
			 strPref, (const char*)m_selfHost, (const char*)m_serverHost,
			 m_nRtcpPort, m_nRtcpServerPort);

	log_dump("    SSRC: 0x%X, server SSRC: 0x%X, RR timer: %s\n",
			 m_nSelfSsrc, m_nServerSsrc, m_pRrTimer ? "ON" : "OFF");

	log_dump("    Packets: recv: %u, invalid: %u, ss: %u, sent rr: %u\n",
			 counter_get(m_nPackets), counter_get(m_nInvalidPackets),
			 counter_get(m_nSrPackets), counter_get(m_nRrSentPackets));
}
