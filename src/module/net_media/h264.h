/*
 *	Carbon/Network MultiMedia Streaming Module
 *	H.264 Codec common definitions and routines
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 10.10.2016 14:51:46
 *	    Initial revision.
 */

#ifndef __NET_MEDIA_H264_H_INCLUDED__
#define __NET_MEDIA_H264_H_INCLUDED__

#include "carbon/carbon.h"

typedef struct {
	uint8_t				nal_unit_type:5;
	uint8_t				nal_ref_idc:2;
	uint8_t				forbidden_zero_bit:1;
} h264_nal_head_t;

typedef struct {
	h264_nal_head_t		h;
	uint8_t				rbsp[];
} h264_nal_t;

typedef struct {
	uint8_t				profile_idc;
	uint8_t				reserved_zero_2bits:2;
	uint8_t				constraint_set5_flag:1;
	uint8_t				constraint_set4_flag:1;
	uint8_t				constraint_set3_flag:1;
	uint8_t				constraint_set2_flag:1;
	uint8_t				constraint_set1_flag:1;
	uint8_t				constraint_set0_flag:1;
	uint8_t				level_idc;
} h264_seq_parameter_set_t;

enum {
	H264_DUMP_NAL_HEAD	= 1,
	H264_DUMP_NAL_RBSP	= 2,

	H264_DUMP_ALL = 0xffffffff
};

#define H264_NAL_TYPE_NON_IDR_SLICE 			0x1
#define H264_NAL_TYPE_DP_A_SLICE 				0x2
#define H264_NAL_TYPE_DP_B_SLICE 				0x3
#define H264_NAL_TYPE_DP_C_SLICE 				0x4
#define H264_NAL_TYPE_IDR_SLICE 				0x5
#define H264_NAL_TYPE_SEI 						0x6
#define H264_NAL_TYPE_SEQ_PARAM 				0x7
#define H264_NAL_TYPE_PIC_PARAM 				0x8
#define H264_NAL_TYPE_ACCESS_UNIT 				0x9
#define H264_NAL_TYPE_END_OF_SEQ 				0xA
#define H264_NAL_TYPE_END_OF_STREAM 			0xB
#define H264_NAL_TYPE_FILLER_DATA 				0xC
#define H264_NAL_TYPE_SEQ_EXTENSION 			0xD
#define H264_NAL_TYPE_PREFIX					0xE
#define H264_NAL_TYPE_SUBSET_SEQ_PARAM			0xF
#define H264_NAL_TYPE_RESERVED16				0x10
#define H264_NAL_TYPE_RESERVED17				0x11
#define H264_NAL_TYPE_RESERVED18				0x12
#define H264_NAL_TYPE_AUX_PIC					0x13	/* 19 */
#define H264_NAL_TYPE_SCALABLE_EXT_SLICE		0x14
#define H264_NAL_TYPE_RESERVED21				0x15
#define H264_NAL_TYPE_RESERVED22				0x16
#define H264_NAL_TYPE_RESERVED23				0x17
#define H264_NAL_TYPE_STAP_A					0x18	/* 24 */
#define H264_NAL_TYPE_STAP_B					0x19
#define H264_NAL_TYPE_MTAP16					0x1A
#define H264_NAL_TYPE_MTAP24					0x1B
#define H264_NAL_TYPE_FU_A						0x1C
#define H264_NAL_TYPE_FU_B						0x1D

/*
 * RTP H264 FU-A, FU-B format
 */
/*typedef struct {
	uint8_t				type:5;
	uint8_t				nri:2;
	uint8_t				f:1;
} rtp_h264_payload_fu_indicator_t;*/

typedef struct {
	uint8_t				type:5;
	uint8_t				reserved:1;
	uint8_t				end:1;
	uint8_t				start:1;
} rtp_h264_payload_fu_header_t;

typedef struct {
	h264_nal_head_t					indicator;
	rtp_h264_payload_fu_header_t	header;
	uint8_t							payload[];
} __attribute__ ((packed)) rtp_h264_payload_fua_t;

typedef struct {
	h264_nal_head_t					indicator;
	rtp_h264_payload_fu_header_t	header;
	uint16_t						don;
	uint8_t							payload[];
} __attribute__ ((packed)) rtp_h264_payload_fub_t;



class CH264
{
	private:
		uint8_t*	m_pBuffer;
		size_t 		m_nSize;

	public:
		CH264();
		virtual ~CH264();

	public:
		void clear();
		result_t load(const char* strFilename);

		void dumph264(int nalLimit = 0, int dumpFlag = H264_DUMP_ALL) const;

	private:
		boolean_t isNalSeparator(const uint8_t* pBuffer) const;
		size_t findNextNalSeparator(const uint8_t *pBuffer, size_t length) const;
		void nalDecode3(uint8_t* pDst, int* pDstLen, const uint8_t* pSrc, int srcLen);
		void dumpNalSeqParam(h264_nal_t* pNal) const;
};

extern const char* strNalType(size_t type);

static inline boolean_t h264_nal_is_slice(uint8_t nalType)
{
	return nalType >= H264_NAL_TYPE_NON_IDR_SLICE && nalType <= H264_NAL_TYPE_IDR_SLICE;
}

static inline boolean_t h264_nal_is_rtp(uint8_t nalType)
{
	return nalType >= H264_NAL_TYPE_STAP_A && nalType <= H264_NAL_TYPE_FU_B;
}


#endif /* __NET_MEDIA_H264_H_INCLUDED__ */
