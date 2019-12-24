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
 *	Revision 1.0, 17.10.2016 13:24:34
 *		Initial revision.
 */

#include "net_media/h264.h"
#include "net_media/rtp.h"

static void rtp_init_seq(rtp_source_t *s, uint16_t seq)
{
	s->base_seq = seq;
	s->max_seq = seq;
	s->bad_seq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
	s->cycles = 0;
	s->received = 0;
	s->received_prior = 0;
	s->expected_prior = 0;
	/* other initialization */
	s->transit = 0;
	s->jitter = 0.0;
}

int rtp_update_seq(rtp_source_t *s, uint16_t seq)
{
	uint16_t udelta = seq - s->max_seq;

	/*
	 * Source is not valid until MIN_SEQUENTIAL packets with
	 * sequential sequence numbers have been received.
	 */
	if (s->probation) {
		/* packet is in sequence */
		if (seq == s->max_seq + 1) {
			s->probation--;
			s->max_seq = seq;
			if (s->probation == 0) {
				rtp_init_seq(s, seq);
				s->received++;
				return 1;
			}
		}
		else {
			s->probation = MIN_SEQUENTIAL - 1;
			s->max_seq = seq;
		}
		return 0;
	}
	else if (udelta < MAX_DROPOUT) {
		/* in order, with permissible gap */
		if (seq < s->max_seq) {
			/*
			 * Sequence number wrapped - count another 64K cycle.
			 */
			s->cycles += RTP_SEQ_MOD;
		}
		s->max_seq = seq;
	}
	else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
		/* the sequence number made a very large jump */
		if (seq == s->bad_seq) {
			/*
			 * Two sequential packets -- assume that the other side
			 * restarted without telling us so just re-sync
			 * (i.e., pretend this was the first packet).
			 */
			rtp_init_seq(s, seq);
		}
		else {
			s->bad_seq = (seq + 1) & (RTP_SEQ_MOD-1);
			return 0;
		}
	} else {
		/* duplicate or reordered packet */
	}
	s->received++;
	return 1;
}

void rtp_boot_source(rtp_source_t* s, uint16_t seq) {
	rtp_init_seq(s, seq);
//	s->max_seq   = seq - 1;   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!1
	s->max_seq   = seq;
	s->probation = MIN_SEQUENTIAL;
}

/******************************************************************************/

static void dumpRtpPayloadFua(h264_nal_t* pNal, size_t length, const char* strPref)
{
	h264_nal_head_t*	pNalHead = (h264_nal_head_t*)(((char*)pNal)+1);

	if ( length < 2 )  {
		log_dump("%sRTP PAYLOAD INVALID (TOO SHORT) LENGTH!\n", strPref);
		return;
	}

	log_dump("%sFragmentaion Nal: %s(%d), F: %d, NRI: %d\n",
			 strPref, strNalType(pNal->h.nal_unit_type), pNal->h.nal_unit_type,
			 pNal->h.forbidden_zero_bit, pNal->h.nal_ref_idc);

	log_dump("%sFU-A header: Type: %s(%d), R: %d, E: %d, S:%d\n\n",
			 strPref, strNalType(pNalHead->nal_unit_type), pNalHead->nal_unit_type,
			 pNalHead->nal_ref_idc&1, (pNalHead->nal_ref_idc>>1)&1,
			 pNalHead->forbidden_zero_bit);
}

static void dumpRtpPayloadSingleNal(h264_nal_t* pNal, size_t length, const char* strPref)
{
	boolean_t 	bDumpData;

	if ( length < 1 )  {
		log_dump("%sRTP PAYLOAD INVALID (TOO SHORT) LENGTH!\n", strPref);
		return;
	}

	bDumpData = pNal->h.nal_unit_type == H264_NAL_TYPE_SEQ_PARAM ||
				pNal->h.nal_unit_type == H264_NAL_TYPE_PIC_PARAM;

	log_dump("%sSingle Nal: type: %s(%d), forbidden_zero_bit: %d, ref_idc: %d\n%s",
			 strPref, strNalType(pNal->h.nal_unit_type), pNal->h.nal_unit_type,
			 pNal->h.forbidden_zero_bit, pNal->h.nal_ref_idc,
			 bDumpData ? "" : "\n");

	if ( bDumpData ) {
		log_dump_bin(pNal->rbsp, length /*length-1 (???????)*/, "%sdata:", strPref);
	}
}

static void dumpRtpPayload(uint8_t* pPayload, size_t length, const char* strPref = "")
{
	h264_nal_t*		pNal = (h264_nal_t*)pPayload;


	switch ( pNal->h.nal_unit_type )  {
		case H264_NAL_TYPE_FU_A:
			dumpRtpPayloadFua(pNal, length, strPref);
			break;

		default:
			dumpRtpPayloadSingleNal(pNal, length, strPref);
	}
}

/*
 * Dump RTP frame
 *
 * 		flags		dump flags, RTP_FRAME_DUMP_xxx
 */
void dumpRtpFrame(const rtp_frame_t* pFrame, int flags)
{
	const rtp_head_t*	pHead = &pFrame->head;
	size_t				cc, i, payload_len;
	int 				version, len;
	uint32_t			fields;
	char 				strBuf[256];

	if ( pFrame->length < rtp_frame_head_length(pFrame) )  {
		log_dump("RTP packet: INVALID HEAD LENGTH: %d bytes\n", pFrame->length);
		return;
	}

	fields = pFrame->head.fields;
	version = RTP_HEAD_VERSION(fields);
	cc = RTP_HEAD_CC(fields);
	payload_len = pFrame->length-rtp_frame_head_length(pFrame);

	log_dump("RTP packet (length=%u bytes):\n", pFrame->length);
	log_dump("    ver: %u (%s), padding: %d, extension: %d, CSRC count: %d, marker: %d\n",
			 version, version == RTP_VERSION ? "valid" : "INVALID",
			 RTP_HEAD_PADDING(fields),
			 RTP_HEAD_EXTENSION(fields), cc,
			 RTP_HEAD_MARKER(fields));
	if ( RTP_HEAD_PADDING(fields) )  {
		/* Padding bytes are present */
		uint8_t*	pPadCount = ((uint8_t*)pHead) + pFrame->length - sizeof(uint8_t);
		log_dump("    padding: %d byte(s)\n", (*pPadCount)&0xff);
	}

	log_dump("    payload size: %u bytes, type: %d, sequence number: %d, timestamp %u, SSRC: %xh\n",
			 payload_len,
			 RTP_HEAD_PAYLOAD_TYPE(fields),
			 RTP_HEAD_SEQUENCE(fields),
			 pHead->timestamp, pHead->ssrc);

	if ( cc > 0 )  {
		len = 0;
		for(i=0; i<cc; i++)  {
			if ( len < (int)sizeof(strBuf) ) {
				len = _tsnprintf(&strBuf[len], sizeof(strBuf)-len, "%s %u",
							   i == 0 ? "," : "", pHead->csrc[i]);
			}
			else {
				break;
			}
		}

		log_dump("    CSRC:%s\n", strBuf);
	}

	if ( flags&RTP_FRAME_DUMP_PAYLOAD ) {
		dumpRtpPayload(((uint8_t*)pHead) + rtp_frame_head_length(pFrame),
					   payload_len, "    ");
	}
}
