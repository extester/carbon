/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP protocol common definitions and routines
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 11.10.2016 12:51:31
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTP_H_INCLUDED__
#define __NET_MEDIA_RTP_H_INCLUDED__

#include "shell/queue.h"
#include "shell/hr_time.h"

#include "carbon/carbon.h"

#define RTP_VIDEO_CLOCK_RATE_DEFAULT		90000

#define RTP_VERSION							2

#define RTP_EXTENSION_HEADER_LENGTH_MAX		128
#define RTP_PAYLOAD_LENGTH_MAX				2048
#define RTP_PADDING_LENGTH_MAX				128
#define RTP_PACKET_LENGTH_MAX	\
	(sizeof(rtp_head_t)+ \
	15*sizeof(uint32_t)+ \
	RTP_EXTENSION_HEADER_LENGTH_MAX+ \
	RTP_PAYLOAD_LENGTH_MAX+ \
	RTP_PADDING_LENGTH_MAX)

typedef struct {
	uint32_t		fields;			/* Various flags and sequence number */
	uint32_t		timestamp;		/* Packet timestamp */
	uint32_t		ssrc;			/* Synchronization source identifier */
	uint32_t		csrc[];			/* Contributing source identifier(s) */
} __attribute__ ((packed)) rtp_head_t;

#define RTP_HEAD_VERSION(__fields)			(((__fields)>>30)&0x3)
#define RTP_HEAD_PADDING(__fields)			(((__fields)>>29)&1)
#define RTP_HEAD_EXTENSION(__fields)		(((__fields)>>28)&1)
#define RTP_HEAD_CC(__fields)				(((__fields)>>24)&0xf)
#define RTP_HEAD_MARKER(__fields)			(((__fields)>>23)&1)
#define RTP_HEAD_PAYLOAD_TYPE(__fields)		(((__fields)>>16&0x7f))
#define RTP_HEAD_SEQUENCE(__fields)			((uint16_t)(((__fields)&0xffff)))

#define RTP_HEAD_MARKER_FIELD				(1<<23)


class CRtpFrameCache;

typedef struct
{
	queue_chain_t		link;			/* Queue link */
	CRtpFrameCache*		pOwner;			/* Parent cache */

	size_t				length;			/* Real frame length (excluding padding bytes) */
	hr_time_t			hrArriveTime;	/* Frame received timestamp */
	uint64_t			nSeqNum;		/* Real sequence number */
	rtp_head_t			head;			/* Frame buffer */
	uint8_t				__buffer__[ALIGN(RTP_PACKET_LENGTH_MAX-sizeof(rtp_head_t), 128)];
} __attribute__ ((packed)) rtp_frame_t;

#define RTP_FRAME_NULL		((rtp_frame_t*)NULL)

static inline size_t rtp_frame_head_length(const rtp_frame_t* pFrame)
{
	size_t	length;

	length = sizeof(rtp_head_t);
	length += RTP_HEAD_CC(pFrame->head.fields) * sizeof(uint32_t);
	if ( RTP_HEAD_EXTENSION(pFrame->head.fields) )  {
		uint32_t*	pExtHead = (uint32_t*)(((uint8_t*)&pFrame->head)+length);

		length += (((*pExtHead)&0xffff)+1)*sizeof(uint32_t);
	}

	return length;
}

/*
 * Per-source state information
 */
typedef struct {
	uint16_t max_seq;        /* highest seq. number seen */
	uint64_t cycles;         /* shifted count of seq. number cycles */
	uint32_t base_seq;       /* base seq number */
	uint32_t bad_seq;        /* last 'bad' seq number + 1 */
	uint32_t probation;      /* sequ. packets till source is valid */
	uint32_t received;       /* packets received */
	uint32_t expected_prior; /* packet expected at last interval */
	uint32_t received_prior; /* packet received at last interval */
	int transit;        /* relative trans time for prev pkt */
	double jitter;         /* estimated jitter */
	/* ... */
	hr_time_t	hrTimelineBase;		/* Base timeline value */
	uint64_t	time_cycles;
	uint32_t	max_time;			/* Max rtp timestamp processed */
	hr_time_t	hrTimeOffset;		/* Sender/receiver timeline offset */
} rtp_source_t;

#define RTP_SEQ_MOD (1<<16)
static const unsigned int MAX_DROPOUT = 3000;
static const int MAX_MISORDER = 100;
static const int MIN_SEQUENTIAL = 0; /* XXX TTT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 2 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

static const unsigned int MAX_DROPOUT_TIMELINE = MAX_DROPOUT/25*90000;
#define RTP_TIMELINE_MOD (1ULL<<32)

extern void rtp_boot_source(rtp_source_t* s, uint16_t seq);
extern int rtp_update_seq(rtp_source_t *s, uint16_t seq);

enum {
	RTP_FRAME_DUMP_HEAD = 0,
	RTP_FRAME_DUMP_PAYLOAD = 1,

	RTP_FRAME_DUMP_ALL = 0xffffffff
};

extern void dumpRtpFrame(const rtp_frame_t* pFrame, int flags = RTP_FRAME_DUMP_ALL);

#endif /* __NET_MEDIA_RTP_H_INCLUDED__ */
