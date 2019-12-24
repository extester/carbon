/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP Playout Buffer for H264 Encoded Video
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 17.10.2016 13:34:56
 *		Initial revision.
 */

#include <new>

#include "carbon/memory.h"

#include "net_media/rtp_playout_buffer_h264.h"

/*******************************************************************************
 * CRtpPlayoutNodeH264 class
 */

CRtpPlayoutNodeH264::CRtpPlayoutNodeH264(uint64_t rtpRealTimestamp, hr_time_t hrPlayoutTime,
										 CRtpPlayoutBuffer* pParent) :
	CRtpPlayoutNode(rtpRealTimestamp, hrPlayoutTime, pParent),
	m_flags(0),
	m_pData(NULL),
	m_nSize(0)
{
}

CRtpPlayoutNodeH264::~CRtpPlayoutNodeH264()
{
	SAFE_FREE(m_pData);
}

/*
 * Insert frame to the existing node
 */
result_t CRtpPlayoutNodeH264::insertFrame(rtp_frame_t* pFrame)
{
	result_t	nresult;

	nresult = CRtpPlayoutNode::insertFrame(pFrame);
	if ( nresult == ESUCCESS )  {
		size_t 		headLen = rtp_frame_head_length(pFrame);
		size_t		payloadLen = pFrame->length - headLen;
		uint8_t*	pPayload = ((uint8_t*)&pFrame->head) + headLen;
		h264_nal_t*	pNal = (h264_nal_t*)pPayload;
		uint8_t		nalType = pNal->h.nal_unit_type;

		shell_assert(!(m_flags&flagReady));

		/*
		 * Save SPS/PPS frame payload
		 */
		if ( nalType == H264_NAL_TYPE_SEQ_PARAM || nalType == H264_NAL_TYPE_PIC_PARAM ) {
			CRtpPlayoutBufferH264*	pParent = (CRtpPlayoutBufferH264*)m_pParent;

			if ( pNal->h.nal_unit_type == H264_NAL_TYPE_SEQ_PARAM ) {
				pParent->setSps(pPayload, payloadLen);
			}
			else {
				pParent->setPps(pPayload, payloadLen);
			}

			m_flags |= flagParams;

			/*if ( payloadLen != 21 && payloadLen !=4 ) {
			log_debug(L_RTP, "[rtp_playout_node_h264(%s)]: !!!!!!!!!!!!!!!!!!!!!!!!!! In-Band %s Parameters set: payloadlen %d bytes\n",
					  	getName(), nalType == H264_NAL_TYPE_SEQ_PARAM ? "SEQ" : "PIC", payloadLen);
			log_dump_bin(pPayload, payloadLen, "%s Data: ", nalType == H264_NAL_TYPE_SEQ_PARAM ? "SEQ" : "PIC");
			log_debug(L_RTP, "[rtp_playout_node_h264(%s)]: !!!!!!!!!!! frame len: %d, head len: %d\n",
					  	getName(), pFrame->length, headLen);
			}*/
		}

		/*
		 * Set last frame flag
		 */
		if ( (h264_nal_is_rtp(nalType) || h264_nal_is_slice(nalType))
				&& RTP_HEAD_MARKER(pFrame->head.fields) ) {
			m_flags |= flagLast;
		}

		if ( checkNodeValid() )  {
			buildCompressed();
		}
	}

	return nresult;
}

/*
 * Check if a node is completed and valid
 *
 * Return: TRUE/FALSE
 *
 * Note: Node is valid if:
 *
 * 		1) all frames are present
 */
boolean_t CRtpPlayoutNodeH264::checkNodeValid() const
{
	const rtp_frame_t*	elt;
	uint16_t			nextSeq;
	boolean_t			bValid;
	boolean_t			bFuaStart, bFuaEnd;

	if ( (m_flags&flagLast) == 0 )  {
		return FALSE;		/* Last frame was not received */
	}

	shell_assert(m_nLength);

	bValid = TRUE;
	bFuaStart = FALSE, bFuaEnd = FALSE;

	elt = (const rtp_frame_t*)queue_first(&m_queue);
	nextSeq = RTP_HEAD_SEQUENCE(elt->head.fields);

	queue_iterate(&m_queue, elt, const rtp_frame_t*, link) {
		size_t 				headLen = rtp_frame_head_length(elt);
		const uint8_t*		pPayload = ((const uint8_t*)&elt->head) + headLen;
		const h264_nal_t*	pNal = (const h264_nal_t*)pPayload;
		const rtp_h264_payload_fua_t*	pFua;

		if ( RTP_HEAD_SEQUENCE(elt->head.fields) != nextSeq ) {
			bValid = FALSE;
			//log_error(L_RTP, "[rtp_playout_node_h264(%s)] -- Node wrong sequence %u (expected %u), dropped --\n",
			//		  getName(), RTP_HEAD_SEQUENCE(elt->head.fields), nextSeq);
			break;
		}
		nextSeq++;

		switch ( pNal->h.nal_unit_type )  {
			case H264_NAL_TYPE_FU_A:
				pFua = (const rtp_h264_payload_fua_t*)pNal;

				if ( pFua->header.start )  {
					if ( bFuaStart || bFuaEnd )  {
						//log_error(L_RTP, "[rtp_playout_node_h264(%s)] -- FUA invalid fragment, (dup Start or End), node dropped --\n",
						//		  getName());
						bValid = FALSE;		/* Duplicated start part */
					}
					else {
						bFuaStart = TRUE;
					}
				} else if ( pFua->header.end )  {
					if ( !bFuaStart || bFuaEnd )  {
						//log_error(L_RTP, "[rtp_playout_node_h264(%s)] -- FUA invalid fragment, (no Start or dup End), node dropped --\n",
						//		  getName());
						bValid = FALSE;		/* Duplicated end part */
					}
					else {
						bFuaEnd = TRUE;
					}
				} else {
					if ( !bFuaStart || bFuaEnd )  {
						//log_error(L_RTP, "[rtp_playout_node_h264(%s)] -- FUA invalid fragment sequence, node dropped --\n",
						//		  	getName());
						bValid = FALSE;
					}
				}
				break;
		}

		if ( !bValid )  {
			break;
		}
	}

	if ( bFuaStart != bFuaEnd )  {
		//log_error(L_RTP, "[rtp_playout_node_h264(%s)] -- FUA invalid fragment S:%d, E:%d, node dropped --\n",
		//		  getName(), bFuaStart, bFuaEnd);
		bValid = FALSE;
	}

	return bValid;
}

#define WRITE_NAL_SEPARATOR(__a, __len)		\
	do { *((uint32_t*)(__a)) = 0x01000000; (__len)+=4; } while(0)

/*
void xdump_nal(const uint8_t* pData, size_t size)
{
	const uint8_t* p, *e, *x;
	uint32_t sep = 0x01000000;
	const h264_nal_head_t*	pNalPrev = 0;

	p = pData;
	e = pData + size;

	log_debug(L_GEN, "====== NAL LIST, %u bytes\n", size);
	while ( p < e )  {
		const h264_nal_head_t*	pNal;

		x = (const uint8_t*)memmem(p, A(e)-A(p)+1, &sep, 4);
		if ( x )  {
			pNal = (const h264_nal_head_t*)(x+4);
			if ( (const uint8_t*)pNal >= e ) {
				log_error(L_GEN, "-------- nal head overflow\n");
				break;
			}

			if ( pNalPrev == 0 )  {
				pNalPrev = pNal;
			}
			else {
				size_t len = A(pNal) - A(pNalPrev) - 4;

				if ( pNalPrev->nal_unit_type == H264_NAL_TYPE_PIC_PARAM )  {
					log_debug(L_GEN, "--- NAL: offset %5u, len: %5u bytes (PPS)\n", A(pNalPrev)-A(pData), len);
				}
				else if ( pNalPrev->nal_unit_type == H264_NAL_TYPE_SEQ_PARAM )  {
					log_debug(L_GEN, "--- NAL: offset %5u, len: %5u bytes (SPS)\n", A(pNalPrev)-A(pData), len);
				}
				else {
					log_debug(L_GEN, "--- NAL: offset %5u, len: %5u bytes (type %d)\n", A(pNalPrev)-A(pData), len, pNalPrev->nal_unit_type);
				}

				pNalPrev = pNal;
			}

			p = x+4;
		}
		else {
			if ( pNalPrev == 0 )  {
				log_error(L_GEN, "------- nal NO SEPARATORS\n");
			}
			else {
				size_t len = A(e) - A(pNalPrev);

				log_debug(L_GEN, "--- NAL: offset %5u, len: %5u bytes (type %d) LAST\n", A(pNalPrev)-A(pData), len, pNalPrev->nal_unit_type);
			}
			break;
		}
	}
}


boolean_t xdump_nal_check(const uint8_t* pData, size_t size, uint32_t flags)
{
	const uint8_t* p, *e, *x;
	uint32_t sep = 0x01000000;
	const h264_nal_head_t*	pNalPrev = 0;
	boolean_t bResult = TRUE;

	p = pData;
	e = pData + size;

	//log_debug(L_GEN, "====== NAL LIST, %u bytes\n", size);
	while ( p < e )  {
		const h264_nal_head_t*	pNal;

		x = (const uint8_t*)memmem(p, A(e)-A(p)+1, &sep, 4);
		if ( x )  {
			pNal = (const h264_nal_head_t*)(x+4);
			if ( (const uint8_t*)pNal >= e ) {
				log_error(L_GEN, "-------- ++++ nal head overflow\n");
				bResult = FALSE;
				break;
			}

			if ( pNalPrev == 0 )  {
				pNalPrev = pNal;
			}
			else {
				size_t len = A(pNal) - A(pNalPrev) - 4;

				if ( pNalPrev->nal_unit_type == H264_NAL_TYPE_PIC_PARAM )  {
					if ( len != 4 ) {
						log_debug(L_GEN, "--- ---- ---- +++ NALCHECK: offset %5u, len: %5u bytes (PPS)\n",
								  A(pNalPrev) - A(pData), len);
						log_debug(L_GEN, "--- --- --- pData %Xh, size %u, flags %u\n", pData, size, flags);

						xdump_nal(pData, size);
						bResult = FALSE;
					}
				} else if ( pNalPrev->nal_unit_type == H264_NAL_TYPE_SEQ_PARAM )  {
					if ( len != 21 ) {
						log_debug(L_GEN, "--- ---- ---- +++ NALCHECK: offset %5u, len: %5u bytes (SPS)\n",
								  A(pNalPrev) - A(pData), len);
						log_debug(L_GEN, "--- --- --- pData %Xh, size %u, flags %u\n", pData, size, flags);

						xdump_nal(pData, size);
						bResult = FALSE;
					}
				}
				else {
					//log_debug(L_GEN, "--- NAL: offset %5u, len: %5u bytes (type %d)\n", A(pNalPrev)-A(pData), len, pNalPrev->nal_unit_type);
				}

				pNalPrev = pNal;
			}

			p = x+4;
		}
		else {
			if ( pNalPrev == 0 )  {
				log_error(L_GEN, "------- nal NO SEPARATORS\n");
				bResult = FALSE;
			}
			else {
				size_t len = A(e) - A(pNalPrev);

				if ( pNalPrev->nal_unit_type == H264_NAL_TYPE_PIC_PARAM )  {
					if ( len != 4 ) {
						log_debug(L_GEN, "--- ---- ---- +++ NALCHECK LAST: offset %5u, len: %5u bytes (PPS)\n",
								  A(pNalPrev) - A(pData), len);
						log_debug(L_GEN, "--- --- --- pData %Xh, size %u, flags %u\n", pData, size, flags);

						xdump_nal(pData, size);
						bResult = FALSE;
					}
				} else if ( pNalPrev->nal_unit_type == H264_NAL_TYPE_SEQ_PARAM ) {
					if ( len != 21 ) {
						log_debug(L_GEN, "--- ---- ---- +++ NALCHECK LAST: offset %5u, len: %5u bytes (SPS)\n",
								  A(pNalPrev) - A(pData), len);
						log_debug(L_GEN, "--- --- --- pData %Xh, size %u, flags %u\n", pData, size, flags);

						xdump_nal(pData, size);
						bResult = FALSE;
					}
				}

				//log_debug(L_GEN, "--- NAL: offset %5u, len: %5u bytes (type %d) LAST\n", A(pNalPrev)-A(pData), len, pNalPrev->nal_unit_type);
			}
			break;
		}
	}

	return bResult;
}
*/


/*
 * Build a compressed H264 image
 *
 * Return: ESUCCESS, ENOMEM
 */
result_t CRtpPlayoutNodeH264::buildCompressed()
{
	const rtp_frame_t*		elt;
	CRtpPlayoutBufferH264*	pParent = (CRtpPlayoutBufferH264*)m_pParent;
	size_t					length, offset;
	boolean_t				bNeedSeparator = FALSE;

	shell_assert(m_nLength > 0);
	shell_assert(m_pData == 0);
	shell_assert(m_nSize == 0);

	/*
	 * Calculate an image length
	 */
	length = 0;

	queue_iterate(&m_queue, elt, const rtp_frame_t*, link) {
		size_t 				headLen = rtp_frame_head_length(elt);
		size_t				payloadLen = elt->length - headLen;
		const uint8_t*		pPayload = ((const uint8_t*)&elt->head) + headLen;
		const h264_nal_t*	pNal = (const h264_nal_t*)pPayload;

		length += 4;		/* nal separator: 00 00 00 01 */

		switch ( pNal->h.nal_unit_type )  {
			case H264_NAL_TYPE_FU_A:
				length += sizeof(h264_nal_head_t) + (payloadLen - sizeof(rtp_h264_payload_fua_t));
				break;

			case H264_NAL_TYPE_STAP_A:
			case H264_NAL_TYPE_STAP_B:
			case H264_NAL_TYPE_MTAP16:
			case H264_NAL_TYPE_MTAP24:
			case H264_NAL_TYPE_FU_B:
				pParent->incrUnsupportedFrame();
				log_error(L_RTP, "[rtp_playout_node_h264(%s)] unsupported NAL type %d\n",
						  	getName(), pNal->h.nal_unit_type);
				return EINVAL;

			default:
				length += payloadLen;
				break;
		}
	}

	if ( (m_flags&flagParams) == 0 ) {
		/*
		 * Write SPS/PPS
		 */
		length += pParent->getSPsize();
	}

	/*
	 * Allocate buffer
	 */
	length = ALIGN(length, 64);
	m_pData = (uint8_t*)memAlloc(length);
	if ( !m_pData )  {
		log_error(L_RTP, "[rtp_playout_node_h264(%s)] out of memory %d bytes\n", getName(), length);
		return ENOMEM;
	}

	/*
	 * Copy H264 data
	 */
	offset = 0;

	/* First write SPS/PPS */
	if ( (m_flags&flagParams) == 0 ) {
		offset += pParent->getSPdata(m_pData+offset);
		bNeedSeparator = TRUE;

		/*if ( offset != 33 ) {
			log_debug(L_RTP, "[rtp_playout_node_h264(%s)] !!!!!!!!!!!!!!!!!! writing SPS/PPS %d bytes\n", getName(), offset);
			log_dump_bin(m_pData, offset, "[rtp_playout_node_h264(%s)] !!! PPS Data: ");
		}*/
	}

	queue_iterate(&m_queue, elt, const rtp_frame_t*, link) {
		size_t 				headLen = rtp_frame_head_length(elt);
		size_t				payloadLen = elt->length - headLen;
		const uint8_t*		pPayload = ((const uint8_t*)&elt->head) + headLen;
		const h264_nal_t*	pNal = (const h264_nal_t*)pPayload;
		const rtp_h264_payload_fua_t*	pFua;
		h264_nal_head_t		nalHead;

		switch ( pNal->h.nal_unit_type )  {
			case H264_NAL_TYPE_FU_A:
				pFua = (const rtp_h264_payload_fua_t*)pNal;
				//log_debug(L_RTP, "build FU_A, offset %d, start: %d\n", offset, pFua->header.start);

				if ( pFua->header.start )  {
					WRITE_NAL_SEPARATOR(m_pData+offset, offset);

					nalHead.nal_unit_type = pFua->header.type;
					nalHead.nal_ref_idc = pFua->indicator.nal_ref_idc;
					nalHead.forbidden_zero_bit = pFua->indicator.forbidden_zero_bit;

					UNALIGNED_MEMCPY(m_pData+offset, &nalHead, sizeof(nalHead));
					offset += sizeof(nalHead);

					/* Check for an IDR frame */
					if ( nalHead.nal_unit_type == H264_NAL_TYPE_IDR_SLICE )  {
						m_flags |= flagIdr;
					}

					/*if ( nalHead.nal_unit_type == H264_NAL_TYPE_PIC_PARAM )  {
						size_t l = payloadLen-sizeof(rtp_h264_payload_fua_t);
						if ( l != 4 )  {
							log_debug(L_RTP, "[rtp_playout_node_h264(%s)] +++++++++++++ PPS len: %d\n",
									  getName(), l);
						}
					}

					if ( nalHead.nal_unit_type == H264_NAL_TYPE_SEQ_PARAM )  {
						size_t l = payloadLen-sizeof(rtp_h264_payload_fua_t);
						if ( l != 21 )  {
							log_debug(L_RTP, "[rtp_playout_node_h264(%s)] +++++++++++++ SPS len: %d\n",
									  getName(), l);
						}
					}*/

					bNeedSeparator = FALSE;
				}

				if ( bNeedSeparator )  {
					WRITE_NAL_SEPARATOR(m_pData+offset, offset);
					bNeedSeparator = FALSE;
				}

				UNALIGNED_MEMCPY(m_pData+offset, &pFua->payload,
								 payloadLen-sizeof(rtp_h264_payload_fua_t));
				offset += payloadLen-sizeof(rtp_h264_payload_fua_t);
				break;

			default:
				//log_debug(L_RTP, "build Sindle, offset %d\n", offset);
				WRITE_NAL_SEPARATOR(m_pData+offset, offset);
				bNeedSeparator = FALSE;

				UNALIGNED_MEMCPY(m_pData+offset, pNal, payloadLen);
				offset += payloadLen;

				/* Check if a IDR frame */
				if ( pNal->h.nal_unit_type == H264_NAL_TYPE_IDR_SLICE )  {
					m_flags |= flagIdr;
				}
				break;
		}
	}

	m_nSize = offset;

	elt = (const rtp_frame_t*)queue_first(&m_queue);
	m_hrPts = elt->hrArriveTime;

	m_flags |= flagReady;

	/*
	boolean_t bbb;
	bbb = xdump_nal_check(m_pData, m_nSize, m_flags);

	if ( !bbb )  {
		int ccc = 0;
		queue_iterate(&m_queue, elt, const rtp_frame_t*, link) {
			ccc++;
		}

		log_debug(L_GEN, "======> QUEUE COUNT %d, flags %Xh\n", ccc, m_flags);

		int n=0;
		queue_iterate(&m_queue, elt, const rtp_frame_t*, link) {
			size_t 				headLen = rtp_frame_head_length(elt);
			size_t				payloadLen = elt->length - headLen;

			log_dump_bin(&elt->head, 32, "============> queue[%d]:  headLen %d, payloadLen: %d ::: ", n, headLen, payloadLen);
			log_dump_bin(((uint8_t*)&elt->head)+32, 32, "");

			log_debug(L_GEN, "----- DUMP RTP, part index %d ----\n", n);
			dumpRtpFrame(elt);
			log_debug(L_GEN, "----- END DUMP RTP, part index %d ----\n", n);

			n++;

		}
	}
	*/

	return ESUCCESS;
}


/*******************************************************************************
 * Debugging support
 */

int CRtpPlayoutNodeH264::dumpPayloadFua(const uint8_t* pPayload, size_t payloadLen,
										 char* strBuf, int strBufLen) const
{
	const rtp_h264_payload_fua_t*	pUnit = (const rtp_h264_payload_fua_t*)pPayload;
	int		length;

	if ( payloadLen < sizeof(*pUnit) )  {
		length = _tsnprintf(strBuf, strBufLen, "FU_A: payload too short 1!\n");
		return length;
	}

	length = _tsnprintf(strBuf, strBufLen, "Frag FU_A: Indicator: %s(%d), S=%d, E=%d",
				   strNalType(pUnit->header.type), pUnit->header.type,
				   pUnit->header.start, pUnit->header.end);

	return length;
}

int CRtpPlayoutNodeH264::dumpPayloadSingle(const uint8_t* pPayload, size_t payloadLen,
										   char* strBuf, int strBufLen) const
{
	const h264_nal_t*	pNal = (const h264_nal_t*)pPayload;
	int 	length;

	if ( payloadLen < sizeof(h264_nal_head_t) )  {
		length = _tsnprintf(strBuf, strBufLen, "Single: payload too short 1!\n");
		return length;
	}

	length = _tsnprintf(strBuf, strBufLen, "Single: Nal: %s(%d)",
					  strNalType(pNal->h.nal_unit_type), pNal->h.nal_unit_type);

	return length;
}

void CRtpPlayoutNodeH264::dumpFramePayload(int index, const rtp_frame_t* pFrame, const char* strMargin) const
{
	size_t 				headLen = rtp_frame_head_length(pFrame);
	size_t				payloadLen = pFrame->length - headLen;
	const uint8_t*		pPayload = ((const uint8_t*)&pFrame->head) + headLen;
	const h264_nal_t*	pNal;

	char				strBuf[256];
	int					strBufLen, l = sizeof(strBuf);

	if ( pFrame->length < (headLen+sizeof(h264_nal_head_t)) )  {
		log_dump("frame%3d: ======= ERROR: frame too short 1!\n", index);
		return;
	}

	strBufLen = _tsnprintf(strBuf, l, "%s   frame%3d:  size: %u, seq %u, mark: %u, ", strMargin,
						 index, (unsigned)pFrame->length, RTP_HEAD_SEQUENCE(pFrame->head.fields),
						 RTP_HEAD_MARKER(pFrame->head.fields));

	if ( strBufLen < l ) {
		pNal = (const h264_nal_t*)pPayload;
		switch ( pNal->h.nal_unit_type )  {
			case H264_NAL_TYPE_FU_A:
				strBufLen += dumpPayloadFua(pPayload, payloadLen, &strBuf[strBufLen], l-strBufLen);
				break;

			case H264_NAL_TYPE_STAP_A:
			case H264_NAL_TYPE_STAP_B:
			case H264_NAL_TYPE_MTAP16:
			case H264_NAL_TYPE_MTAP24:
			case H264_NAL_TYPE_FU_B:
				strBufLen += _tsnprintf(&strBuf[strBufLen], l-strBufLen,
								"Nal type: %s(%d) ====== ERROR: UNSUPPORTED ======",
								strNalType(pNal->h.nal_unit_type),
								pNal->h.nal_unit_type);
				break;

			default:
				strBufLen += dumpPayloadSingle(pPayload, payloadLen, &strBuf[strBufLen], l-strBufLen);
				break;
		}
	}

	log_dump("%s\n", strBuf);
}

void CRtpPlayoutNodeH264::dumpFrames(const char* strMargin) const
{
	const rtp_frame_t*	elt;
	int 				n = 0;

	queue_iterate(&m_queue, elt, const rtp_frame_t*, link) {
		dumpFramePayload(n, elt, strMargin);
		n++;
	}

}

void CRtpPlayoutNodeH264::dump(const char* strPref) const
{
	CRtpPlayoutNode::dump(strPref);

	log_dump("%s>> Flags: %u, Completed: %d bytes\n", strPref, m_flags, m_nSize);

	dumpFrames(strPref);
}


/*******************************************************************************
 * CRtpPlayoutBufferH264 class
 */

CRtpPlayoutBufferH264::CRtpPlayoutBufferH264(int nProfile, int nFps, int nClockRate,
							int nMaxInputQueue, int nMaxDelay, const char* strName) :
	CRtpPlayoutBuffer(nProfile, nFps, nClockRate, nMaxInputQueue, nMaxDelay, strName),
	m_nSps(0),
	m_nPps(0)
{
	counter_set(m_nUnsupportedFrames, 0);
}

CRtpPlayoutBufferH264::~CRtpPlayoutBufferH264()
{
}

/*
 * Check a H264 frame payload
 *
 * 		pFrame		frame to check
 *
 * Return: TRUE/FALSE
 */
boolean_t CRtpPlayoutBufferH264::validateFrame(const rtp_frame_t* pFrame) const
{
	size_t 				headLen = rtp_frame_head_length(pFrame);
	size_t				payloadLen = pFrame->length - headLen;
	const uint8_t*		pPayload = ((const uint8_t*)&pFrame->head) + headLen;
	const h264_nal_t*	pNal;
	boolean_t			bValid;

	if ( headLen >= pFrame->length ) {
		return FALSE;		/* Partial frame received */
	}

	if ( payloadLen < sizeof(h264_nal_head_t) )  {
		return FALSE;		/* Partial frame received */
	}

	bValid = TRUE;

	pNal = (const h264_nal_t*)pPayload;
	switch ( pNal->h.nal_unit_type )  {
		case H264_NAL_TYPE_FU_A:
			if ( payloadLen < sizeof(rtp_h264_payload_fua_t) ) {
				bValid = FALSE;			/* Partial frame received */
			}
			break;

		case H264_NAL_TYPE_STAP_A:
		case H264_NAL_TYPE_STAP_B:
		case H264_NAL_TYPE_MTAP16:
		case H264_NAL_TYPE_MTAP24:
		case H264_NAL_TYPE_FU_B:
			bValid = FALSE;				/* Unsupported frame */
			break;

		default:
			;
	}

	return bValid;
}

/*
 * Create a new node containing a given frame
 *
 * 		pFrame				initial node frame
 * 		rtpRealTimestamp	frame real RTP timestamp
 *
 * Return: node pointer or 0 on any error
 */
CRtpPlayoutNode* CRtpPlayoutBufferH264::createNode(rtp_frame_t* pFrame, uint64_t rtpRealTimestamp)
{
	CRtpPlayoutNode*	pNode = 0;

	if ( validateFrame(pFrame) )  {
		/*
		 * Calculate node's playout time
		 */
		hr_time_t	hrPlayoutTime;
		result_t	nr;

		hrPlayoutTime = pFrame->hrArriveTime + HR_1SEC/m_nFps;

		try {
			pNode = new CRtpPlayoutNodeH264(rtpRealTimestamp, hrPlayoutTime, this);
			nr = pNode->insertFrame(pFrame);
			if ( nr != ESUCCESS )  {
				pNode->release();
				pNode = 0;
			}
		}
		catch(std::bad_alloc& exc) {}
	}
	else {
		log_error(L_RTP, "[rtp_playout_h264(%s)] frame validation failed, FRAME DROPPED\n", getName());
	}

	return pNode;
}

void CRtpPlayoutBufferH264::setSps(void* pData, size_t size)
{
	size_t	rSize = MIN(size, sizeof(m_Sps));

	UNALIGNED_MEMCPY(m_Sps, pData, rSize);
	m_nSps = rSize;
}


void CRtpPlayoutBufferH264::setPps(void* pData, size_t size)
{
	size_t	rSize = MIN(size, sizeof(m_Pps));

/*if ( size > 30 ) {
	log_error(L_GEN, "[rtp_playout_h264(%s)] -----------!!!!!!!!!!!!!!!!!!!!!!!!!-----------\n", getName());
	log_error(L_GEN, "[rtp_playout_h264(%s)] ----- RTP PPS length %d\n", getName(), size);
}

if ( m_nPps != size )  {
	log_error(L_RTP, "[rtp_playout_h264(%s)] Picture Parameter Set (PPS) duplicated, old size %d, new size %d\n",
			  		getName(), m_nPps, size);
}*/

	UNALIGNED_MEMCPY(m_Pps, pData, rSize);
	m_nPps = rSize;
}

size_t CRtpPlayoutBufferH264::getSPdata(void* pBuffer) const
{
	uint8_t*	pBuf = (uint8_t*)pBuffer;
	size_t		offset = 0;

	if ( m_nSps > 0 )  {
		WRITE_NAL_SEPARATOR(pBuf+offset, offset);
		UNALIGNED_MEMCPY(pBuf+offset, m_Sps, m_nSps);
		offset += m_nSps;
	}
	if ( m_nPps > 0 )  {
		WRITE_NAL_SEPARATOR(pBuf+offset, offset);
		UNALIGNED_MEMCPY(pBuf+offset, m_Pps, m_nPps);
		offset += m_nPps;
	}

	return offset;
}

size_t CRtpPlayoutBufferH264::getSPsize() const
{
	size_t	size = m_nSps+m_nPps;
	size += (m_nSps>0)*4 + (m_nPps>0)*4;
	return size;
}

result_t CRtpPlayoutBufferH264::init(CVideoSink* pSink)
{
	result_t	nresult;

	nresult = CRtpPlayoutBuffer::init(pSink);
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	shell_assert(m_nFps > 0);
	m_nSps = m_nPps = 0;

	return nresult;
}

void CRtpPlayoutBufferH264::terminate()
{
	CRtpPlayoutBuffer::terminate();
}

/*******************************************************************************
 * Debugging support
 */

void CRtpPlayoutBufferH264::dump(const char* strPref) const
{
	CRtpPlayoutBuffer::dump(strPref);
	log_dump("    FPS: %d, SPS: %d bytes, PPS: %d bytes, Unsupported frames: %d\n",
			 m_nFps, m_nSps, m_nPps, counter_get(m_nUnsupportedFrames));

	//dumpNodes();
}
