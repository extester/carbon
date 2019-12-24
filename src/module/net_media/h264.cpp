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
/*
 *	H264 Stream:
 * 	+-----------------+------------------+-------------+-------- ...
 * 	| sep (3/4 bytes) | NAL (n bytes)    | sep         | NAL
 * 	+-----------------+------------------+-------------+-------- ...
 *
 * 	NAL:
 * 	+---------------------+----------------------------------+
 * 	| NAL Header (1 byte) | Raw Byte Sequence Payload (RBSP) |
 *	+---------------------+----------------------------------+
 *
 */
#include "shell/file.h"

#include "carbon/memory.h"

#include "net_media/h264.h"

#define H264_NAL_SEPARATOR			0x00000001

//#define H264_NAL_TYPE(__nal_head)	((__nal_head)&0x1F)

const char* strNalType(size_t type)
{
	static const char* arNalTypes[] = {
		/* 00 */	"Undefined",
		/* 01 */	"Coded slice of a non-IDR pic",
		/* 02 */	"Coded slice data partition A",
		/* 03 */	"Coded slice data partition B",
		/* 04 */	"Coded slice data partition C",
		/* 05 */	"Coded slice of an IDR pic (IDR)",
		/* 06 */	"Additional information (SEI)",
		/* 07 */	"Sequence parameter set (SPS)",
		/* 08 */	"Picture parameter set (PPS)",
		/* 09 */ 	"Access unit delimiter",
		/* 10 */	"End of sequence",
		/* 11 */	"End of stream",
		/* 12 */	"Filler data",
		/* 13 */	"Sequence extension",
		/* 14 */	"Prefix NAL unit",
		/* 15 */	"Subset sequence parameter set",
		/* 16 */	"-Reserved16-",
		/* 17 */	"-Reserved17-",
		/* 18 */	"-Reserved18-",
		/* 19 */	"Coded slice of an aux coded pic without part",
		/* 20 */	"Coded slice in scalable extension",
		/* 21 */	"-Reserved21-",
		/* 22 */	"-Reserved22-",
		/* 23 */	"-Reserved23-",
		/* 24 */	"Single-time aggregation (STAP-A)",
		/* 25 */	"Single-time aggregation (STAP-B)",
		/* 26 */	"Multi-time aggregation (MTAP16)",
		/* 27 */	"Multi-time aggregation (MTAP24)",
		/* 28 */	"Fragmentation (FU-A)",
		/* 29 */	"Fragmentation (FU-B)"
	};

	return type <= ARRAY_SIZE(arNalTypes) ? arNalTypes[type] : "*ERROR*";
}

/*******************************************************************************
 * CH264 class
 */

CH264::CH264() :
	m_pBuffer(NULL),
	m_nSize(0)
{
}

CH264::~CH264()
{
	clear();
}

void CH264::clear()
{
	SAFE_FREE(m_pBuffer);
	m_nSize = 0;
}

result_t CH264::load(const char* strFilename)
{
	CFile		file;
	uint64_t	size64;
	size_t		size;
	result_t	nresult;

	nresult = file.open(strFilename, CFile::fileRead);
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	nresult = file.getSize(&size64);
	if ( nresult != ESUCCESS )  {
		file.close();
		return nresult;
	}

	clear();
	m_pBuffer = (uint8_t*)memAlloc(size64);
	if ( !m_pBuffer )  {
		file.close();
		return ENOMEM;
	}

	m_nSize = (size_t)size64;

	size = m_nSize;
	nresult = file.read(m_pBuffer, &size, CFile::fileReadFull);
	file.close();
	if ( nresult != ESUCCESS )  {
		clear();
	}

	return nresult;
}

boolean_t CH264::isNalSeparator(const uint8_t *pBuffer) const
{
	return pBuffer[0] == 0 && pBuffer[1] == 0 &&
			( (pBuffer[2] == 1) ||
		 	( (pBuffer[2] == 0) && pBuffer[3] == 1) );
}

size_t CH264::findNextNalSeparator(const uint8_t *pBuffer, size_t length) const
{
	uint32_t 	val, temp;
	size_t		offset;

	offset = 0;
	if ( isNalSeparator(pBuffer) ) {
		pBuffer += 3;
		offset = 3;
	}

	val = 0xffffffff;
	while ( offset < length-3 ) {
		val <<= 8;
		temp = val & 0xff000000;
		val &= 0x00ffffff;
		val |= *pBuffer++;
		offset++;
		if ( val == H264_NAL_SEPARATOR ) {
			if ( temp == 0 ) return offset-4;
			return offset-3;
		}
	}

	return 0;
}

void CH264::nalDecode3(uint8_t* pDst, int* pDstLen, const uint8_t* pSrc, int srcLen)
{
	uint8_t*		svDst = pDst;
	const uint8_t*	end = &pSrc[srcLen];

	while ( pSrc < end ) {
		if ( pSrc < end - 3 && pSrc[0] == 0x00 && pSrc[1] == 0x00 && pSrc[2] == 0x03) {
			*pDst++ = 0x00;
			*pDst++ = 0x00;

			pSrc += 3;
			continue;
		}

		*pDst++ = *pSrc++;
	}

	*pDstLen = (int)(A(pDst) - A(svDst));
}



void CH264::dumpNalSeqParam(h264_nal_t* pNal) const
{
	const h264_seq_parameter_set_t*	pParam = (const h264_seq_parameter_set_t*)pNal->rbsp;

	log_dump("      RBSP: profile_idc: %d, constraint_setX_flag: %d %d %d %d %d %d, level_idc: %d\n",
			 pParam->profile_idc, pParam->constraint_set0_flag,
			 pParam->constraint_set1_flag, pParam->constraint_set2_flag,
			 pParam->constraint_set3_flag, pParam->constraint_set4_flag,
			 pParam->constraint_set5_flag, pParam->level_idc);
}


void CH264::dumph264(int nalLimit, int dumpFlag) const
{
	h264_nal_t*	pNal;
	size_t		offset = 0, sepOff, sepLen, nalLen;
	int 		nals = 0;

	shell_assert(m_nSize);

	log_dump("H264 stream dump (%u bytes):\n", m_nSize);
	if ( m_nSize < 4 )  {
		log_dump("H264 stream is too short!\n");
		return;
	}

	nals = 0;
	do {
		sepOff = offset;
		if ( offset == 0 )  {
			if ( isNalSeparator(m_pBuffer) )  {
				sepOff = 0;
			}
			else {
				sepOff = findNextNalSeparator(m_pBuffer, m_nSize);
			}
		}

		sepLen = m_pBuffer[sepOff+2] == 1 ? 3 : 4;
		pNal = (h264_nal_t*)&m_pBuffer[sepOff+sepLen];

		offset = findNextNalSeparator(&m_pBuffer[sepOff], m_nSize-sepOff);
		if ( offset ) { offset += sepOff; }
		nalLen = offset ? (offset-(sepOff+sepLen)) : (m_nSize-(sepLen+sepLen));

		log_dump(" NAL: offset %u, length %u bytes\n", sepOff+sepLen, nalLen);

		if ( dumpFlag&H264_DUMP_NAL_HEAD ) {
			log_dump("      Head: type: %s(%d), forbidden_zero_bit: %d, ref_idc: %d\n",
					 strNalType(pNal->h.nal_unit_type), pNal->h.nal_unit_type,
					 pNal->h.forbidden_zero_bit, pNal->h.nal_ref_idc);
		}

		if ( dumpFlag&H264_DUMP_NAL_RBSP )  {
			if ( pNal->h.nal_unit_type == H264_NAL_TYPE_SEQ_PARAM )  {
				dumpNalSeqParam(pNal);
			}
		}

		nals++;
		if ( nalLimit != 0 && nalLimit <= nals ) {
			break;
		}
	} while (offset != 0);

	log_dump(" -- found %d Nal(s)\n", nals);
}
