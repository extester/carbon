/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTCP protocol common definitions and routines
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 08.11.2016 16:49:10
 *		Initial revision.
 */

#include "net_media/rtcp.h"

static const char* strPacketType(int type)
{
	static const char* ar[] = {
		"Sender Report SR",
		"Receiver Report RR",
		"Source Description SDES",
		"Membership Control BYE",
		"Application Defined APP"
	};

	if ( type >= RTCP_PACKET_SR && type <= RTCP_PACKET_APP )  {
		return ar[type-RTCP_PACKET_SR];
	}

	return "*Unknown*";
}

static const char* strSdesType(int type)
{
	static const char* ar[] = {
		"**ERROR**",
		"CNAME",
		"NAME",
		"EMAIL",
		"PHONE",
		"LOC",
		"TOOL",
		"NOTE",
		"PRIV"
	};

	return type < (int)ARRAY_SIZE(ar) ? ar[type] : "**UNKNOWN**";
}

static void dumpReportBlock(const rtcp_report_block_t* pBlock, const char* strPref)
{
	log_dump("%s   report: reportee SSRC: 0x%X, Loss fraction: %u, Packets lost: %ld, Highest Seq: %lu\n",
			 	strPref, pBlock->reporteeSsrc, RTCP_REPORT_BLOCK_FRACTION_LOSS(pBlock->lost),
			 	RTCP_REPORT_BLOCK_PACKETS_LOST(pBlock->lost), pBlock->highSequence);

	log_dump("%s          Jitter: %d, LSR: %lu, DLSR: %lu\n",
			 	strPref, pBlock->jitter, pBlock->lsr, pBlock->dlsr);
}

static void dumpPacketSr(const rtcp_sr_packet_t* pPacket, const char* strPref)
{
	int 	i, count;

	log_dump("%s   SSRC: 0x%X, NTP time: %lld, RTP time: %u, packets: %u, bytes: %u\n",
			 strPref, pPacket->ssrc, pPacket->ntp_timestamp, pPacket->rtp_timestamp,
			 pPacket->packet_count, pPacket->octet_count);

	count = RTCP_HEAD_ITEM_COUNT(pPacket->h.head);
	for(i=0; i<count; i++)  {
		dumpReportBlock(&pPacket->rb[i], strPref);
	}
}

static void dumpPacketRr(const rtcp_rr_packet_t* pPacket, const char* strPref)
{
	int 	i, count;

	log_dump("%s   Reporter SSRC: 0x%X\n", strPref, pPacket->ssrc);

	count = RTCP_HEAD_ITEM_COUNT(pPacket->h.head);
	for(i=0; i<count; i++)  {
		dumpReportBlock(&pPacket->rb[i], strPref);
	}
}

static int dumpSdesData(const rtcp_sdes_data_t* pData, const char* strPref)
{
	int 			length = 0;
	const char*		s = pData->list;
	char 			type, len, strBuf[256];

	log_dump("%s   SDES SSRC: 0x%X\n", strPref, pData->ssrc);

	while ( *s != '\0' ) {
		type = *s;
		len = *(s+1);

		copySubstring(strBuf, s+2, len, sizeof(strBuf));
		log_dump("%s     > '%s'(%d, %d): '%s'\n", strPref,
				 strSdesType(type), type, len, strBuf);

		length += 2+len;
		s += 2+len;
	}

	length++;
	length = ALIGN(length, sizeof(uint32_t));
	return length;
}

static void dumpPacketSdes(const rtcp_sdes_packet_t* pPacket, const char* strPref)
{
	int 	i, count, size = 0;

	count = RTCP_HEAD_ITEM_COUNT(pPacket->h.head);
	for(i=0; i<count; i++)  {
		size += dumpSdesData((const rtcp_sdes_data_t*)(
			((uint8_t*)pPacket->sdes)+size), strPref);
	}
}

static void dumpPacketBye(const rtcp_bye_packet_t* pPacket, const char* strPref)
{
	int 		i, count, len, length;
	const char*	s;
	char		strBuf[256];

	count = RTCP_HEAD_ITEM_COUNT(pPacket->h.head);
	for(i=0; i<count; i++)  {
		log_dump("%s   SSRC: 0x%X\n", strPref, pPacket->ssrc[i]);
	}

	length = RTCP_HEAD_LENGTH(pPacket->h.head)*sizeof(uint32_t);
	len = count*sizeof(uint32_t);
	if ( length > (len+1) )  {
		s = ((const char*)pPacket) + sizeof(rtcp_bye_packet_t) + len;
		copySubstring(strBuf, s+1, *s, sizeof(strBuf));
		log_dump("%s   Reason: '%s'\n", strPref, strBuf);
	}
}

static void dumpPacketApp(const rtcp_app_packet_t* pPacket, const char* strPref)
{
	char	strBuf[16];
	int 	length;

	copySubstring(strBuf, pPacket->name, sizeof(uint32_t), sizeof(strBuf));
	log_dump("%s   SSRC: 0x%X, Name: '%s'\n", strPref, pPacket->ssrc, strBuf);

	length = RTCP_HEAD_LENGTH(pPacket->h.head)*sizeof(uint32_t)-sizeof(uint32_t)*2;
	log_dump_bin(&pPacket->data, length, "%s   Data (%d bytes):", strPref, length);
}

/*
 * Dump a single RTCP packet
 *
 * 		pPacket		packet pointer
 * 		strPref		dump string prefix
 */
void dumpRtcpPacket(const union rtcp_packet* pPacket, const char* strPref)
{
	int 	type = RTCP_HEAD_PACKET_TYPE(pPacket->h.head);
	int 	version = RTCP_HEAD_VERSION(pPacket->h.head);

	log_dump("%sRTCP Packet: %s(%d), ver: %d (%s), Padding: %s, ItemCount: %d, length: %d dwords\n",
			 strPref, strPacketType(type), type, version,
			 version == RTCP_VERSION ? "VALID" : "INVALID!!!",
			 RTCP_HEAD_PADDING(pPacket->h.head) ? "YES" : "NO",
			 RTCP_HEAD_ITEM_COUNT(pPacket->h.head),
			 RTCP_HEAD_LENGTH(pPacket->h.head));

	switch ( type )  {
		case RTCP_PACKET_SR:
			dumpPacketSr(&pPacket->sr, strPref);
			break;

		case RTCP_PACKET_RR:
			dumpPacketRr(&pPacket->rr, strPref);
			break;

		case RTCP_PACKET_SDES:
			dumpPacketSdes(&pPacket->sdes, strPref);
			break;

		case RTCP_PACKET_BYE:
			dumpPacketBye(&pPacket->bye, strPref);
			break;

		case RTCP_PACKET_APP:
			dumpPacketApp(&pPacket->app, strPref);
			break;

		default:
			;
	}
}

/*
 * Dump a compound RTCP packet
 *
 * 		pPacket		packet pointer
 * 		length		full compaund packet length, bytes
 */
void dumpRtcpCompoundPacket(const void* pData, size_t length)
{
	const uint8_t*				p = (const uint8_t*)pData, *end = p+length, *next;
	const union rtcp_packet* 	pPacket;

	log_dump("*** Compound RTCP packet: %d (bytes)\n", length);

	while ( p < end )  {
		pPacket = (const union rtcp_packet*)p;
		next = p+sizeof(rtcp_head_t)+RTCP_HEAD_LENGTH(pPacket->h.head)*sizeof(uint32_t);
		if ( next > end )  {
			log_error(L_RTCP, "[rtcp] compound packet too short!\n");
			break;
		}

		dumpRtcpPacket((const union rtcp_packet*)p, "    ");
		p = next;
	}
}
