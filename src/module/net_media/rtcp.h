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
 *	Revision 1.0, 07.11.2016 16:39:21
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTCP_H_INCLUDED__
#define __NET_MEDIA_RTCP_H_INCLUDED__

#include "carbon/carbon.h"

#define RTCP_VERSION						2			/* RTCP proto version */

#define RTCP_PACKET_SR						200			/* Sender Report */
#define RTCP_PACKET_RR						201			/* Receiver Report */
#define RTCP_PACKET_SDES					202			/* Source description */
#define RTCP_PACKET_BYE						203			/* Membership control */
#define RTCP_PACKET_APP						204			/* Application-defined */

#define RTCP_HEAD_VERSION(__fields)			(((__fields)>>30)&0x3)
#define RTCP_HEAD_PADDING(__fields)			(((__fields)>>29)&1)
#define RTCP_HEAD_ITEM_COUNT(__fields)		(((__fields)>>24)&0x1f)
#define RTCP_HEAD_PACKET_TYPE(__fields)		(((__fields)>>16&0xff))
#define RTCP_HEAD_LENGTH(__fields)			((__fields)&0xffff)


#define RTCP_SDES_ITEM_CNAME				1			/* Canonical name */
#define RTCP_SDES_ITEM_NAME					2			/* Participant name */
#define RTCP_SDES_ITEM_EMAIL				3			/* Participant email */
#define RTCP_SDES_ITEM_PHONE				4			/* Participant phone */
#define RTCP_SDES_ITEM_LOC					5			/* Participant location */
#define RTCP_SDES_ITEM_TOOL					6			/* RTP implementation tool */
#define RTCP_SDES_ITEM_NOTE					7			/* Participant brief statement */
#define RTCP_SDES_ITEM_PRIV					8			/* Application-specific SDES extension */

typedef struct {
	uint32_t		head;
} rtcp_head_t;

/*
 * RTCP RR Packet
 */
typedef struct {
	uint32_t		reporteeSsrc;				/* Reportee SSRC */
	uint32_t		lost;						/* Loss fraction/Cumulative number of packets lost */
	uint32_t		highSequence;				/* Extended highest sequence number received */
	uint32_t		jitter;						/* Interarrival jitter */
	uint32_t		lsr;						/* Timestamp of last sender report received */
	uint32_t		dlsr;						/* Delay since last sender report received */
} __attribute__ ((packed)) rtcp_report_block_t;

typedef struct {
	rtcp_head_t				h;					/* Common RTCP header */

	uint32_t				ssrc;				/* Reporter SSRC */
	rtcp_report_block_t		rb[];				/* Report blocks */
} __attribute__ ((packed)) rtcp_rr_packet_t;

static inline int32_t RTCP_REPORT_BLOCK_PACKETS_LOST(uint32_t lost) {
	uint32_t	value = lost&0xffffff;
	if ( value&0x800000 )  {
		value |= 0xFF000000;
	}

	return (int32_t)value;
}
#define RTCP_REPORT_BLOCK_FRACTION_LOSS(__lost)		(((__lost)>>24)&0xff)

#define RTCP_REPORT_BLOCK_FORMAT_LOST(__count, __fraction)	\
	((__count) | ((__fraction)<<24))

/*
 * RTCP SR Packet
 */
typedef struct {
	rtcp_head_t				h;					/* Common RTCP header */

	uint32_t				ssrc;				/* Reporter SSRC */
	uint64_t				ntp_timestamp;		/* NTP timestamp */
	uint32_t				rtp_timestamp;		/* RTP timestamp */
	uint32_t				packet_count;		/* Sender's packet count */
	uint32_t				octet_count;		/* Sender's octet count */

	rtcp_report_block_t		rb[];				/* Report blocks */
} __attribute__ ((packed)) rtcp_sr_packet_t;


/*
 * RTCP SDES Packet
 */
typedef struct {
	uint32_t				ssrc;				/* Reporter SSRC */
	char					list[];				/* SDES items */
} __attribute__ ((packed)) rtcp_sdes_data_t;

typedef struct {
	rtcp_head_t				h;					/* Common RTCP header */

	rtcp_sdes_data_t		sdes[];				/* Description of the single source */
} __attribute__ ((packed)) rtcp_sdes_packet_t;


/*
 * RTCP BYE Packet
 */
typedef struct {
	rtcp_head_t				h;					/* Common RTCP header */

	uint32_t				ssrc[];				/* List of ssrc idents */
	/* Reason for leaving (optional) */
} __attribute__ ((packed)) rtcp_bye_packet_t;


/*
 * RTCP APP Packet
 */
typedef struct {
	rtcp_head_t				h;					/* Common RTCP header */

	uint32_t				ssrc;				/* Reporter SSRC */
	char					name[4];			/* Application defined packet name */
	uint8_t					data[];				/* Application defined data */
} __attribute__ ((packed)) rtcp_app_packet_t;


union rtcp_packet
{
	rtcp_head_t				h;

	rtcp_rr_packet_t		rr;
	rtcp_sr_packet_t		sr;
	rtcp_sdes_packet_t		sdes;
	rtcp_bye_packet_t		bye;
	rtcp_app_packet_t		app;
};


extern void dumpRtcpPacket(const union rtcp_packet* pPacket, const char* strPref = "   ");
extern void dumpRtcpCompoundPacket(const void* pData, size_t length);

#endif /* __NET_MEDIA_RTCP_H_INCLUDED__ */
