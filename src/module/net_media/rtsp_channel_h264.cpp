/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTSP protocol H264 video channel
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 25.10.2016 17:31:36
 *		Initial revision.
 */

#include "contact/base64.h"

#include "net_media/rtsp_client.h"
#include "net_media/rtsp_channel_h264.h"

#define RTSP_H264_CHANNEL_MEDIA_TYPE			"video"
#define RTSP_H264_CHANNEL_PROTO_TYPE			"RTP/AVP"
#define RTSP_H264_CHANNEL_PROFILE				96

#define RTSP_H264_CHANNEL_ENCODING				"H264"
#define RTSP_H264_CHANNEL_CLOCKRATE				90000

#define RTSP_H264_CHANNEL_NETTYPE				"IN"
#define RTSP_H264_CHANNEL_ADDRTYPE				"IP4"

#define RTSP_H264_CHANNEL_CAST					"unicast"

#define RTSP_H264_INPUT_QUEUE_LIMIT				250

/*******************************************************************************
 * CRtspChannelH264 class
 */

CRtspChannelH264::CRtspChannelH264(uint32_t id, ip_port_t nRtpPort, int nFps,
						int nRtpMaxDelay, CVideoSink* pSink, const char* strName) :
	CRtspChannel(id, nRtpPort, new CRtpPlayoutBufferH264(RTSP_H264_CHANNEL_PROFILE,
									nFps, RTSP_H264_CHANNEL_CLOCKRATE,
									RTSP_H264_INPUT_QUEUE_LIMIT, nRtpMaxDelay, strName),
				 pSink, strName),
	m_nSps(0),
	m_nPps(0)
{
}

CRtspChannelH264::~CRtspChannelH264()
{
}

/*
 * Reset channel state to uninitialised
 */
void CRtspChannelH264::reset()
{
	CRtspChannel::reset();
	m_nSps = m_nPps = 0;
}

size_t CRtspChannelH264::unpackBase64(const char* pSrc, size_t nSrcLength,
										 uint8_t* pDst, size_t nDstMax)
{
	CBase64		base64;
	size_t		size;
	result_t	nresult;

	nresult = base64.decode(pSrc, nSrcLength);
	if ( nresult != ESUCCESS )  {
		log_debug(L_RTSP, "[rtsp_h264_ch(%s)] failed to base64 decode, result: %d\n",
				  	m_strName.cs(), nresult);
		return 0;
	}

	size = base64.getSize();
	if ( size > nDstMax )  {
		log_warning(L_RTSP, "[rtsp_h264_ch(%s)] base64: decode buffer OVERFLOW!\n",
					m_strName.cs());
		return 0;
	}

	UNALIGNED_MEMCPY(pDst, base64.getData(), size);
	return size;
}

/*
 * Parse H264 parameters from SDP media description
 *
 * 		nMediaIndex			media index in SDP
 * 		nProfile			selected media profile
 * 		pSdp				SDP object pointer
 *
 * Parameters:
 * 		a=control:<trackURL|*>
 * 		a=fmtp:<profile> packetization-mode=<n>;sprop-parameeter-set=<SPS-BASE64>,<PPS-BASE64>
 */
boolean_t CRtspChannelH264::getMediaParams(int nMediaIndex, int nProfile, const CSdp* pSdp)
{
	char 			strTemp[64];
	result_t		nr;

	CString			strEncoding, strParams;
	int 			nClockRate;

	CString			strNetType, strAddrType;
	CNetHost		baseAddr;
	int 			ttl, numAddr;

	CString			strControl;
	CString			strFmtp;

	/* Check encoding/clock-rate */
	nr = pSdp->getMediaRtpmap(nMediaIndex, nProfile, strEncoding, nClockRate, strParams);
	if ( nr != ESUCCESS )  {
		return FALSE;
	}

	if ( strEncoding != RTSP_H264_CHANNEL_ENCODING ||
			nClockRate != RTSP_H264_CHANNEL_CLOCKRATE ) {
		return FALSE;
	}

	/* Check connection information */
	nr = pSdp->getMediaConnectionInfo(nMediaIndex, strNetType, strAddrType, baseAddr, ttl, numAddr);
	if ( nr != ESUCCESS )  {
		return FALSE;
	}

	if ( strNetType != RTSP_H264_CHANNEL_NETTYPE ||
			strAddrType != RTSP_H264_CHANNEL_ADDRTYPE )  {
		return FALSE;
	}

	/* Track specific URL */
	m_strUrl = "";
	nr = pSdp->getMediaAttr(nMediaIndex, "control:", strControl);
	if ( nr == ESUCCESS )  {
		/* Track specific URL was found */
		if ( strControl != "*" ) {
			m_strUrl = strControl;
		}
	}

	/* Packetization, SPS, PPS */
	m_nSps = m_nPps = 0;

	_tsnprintf(strTemp, sizeof(strTemp), "fmtp:%u ", nProfile);
	nr = pSdp->getMediaAttr(nMediaIndex, strTemp, strFmtp);
	if ( nr == ESUCCESS ) {
		CString		strValue;
		int 		iValue;
		boolean_t	bFound;

		bFound = strParseSemicolonKeyValue(strFmtp, "packetization-mode", strValue, " ");
		if ( bFound )  {
			if ( strValue.getNumber(iValue) != ESUCCESS ||
				(iValue != SDP_PACKETIZATION_SINGLE && iValue != SDP_PACKETIZATION_NON_INTERLEAVE) )
			{
				log_debug(L_RTSP, "[rtsp_h264_ch(%s)] media %d: packetization mode '%s' unsupported\n",
						  			m_strName.cs(), nMediaIndex, (const char*)strValue);
				return FALSE;
			}
			log_debug(L_RTSP_FL, "[rtsp_h264_ch(%s)] media %d: packetization mode %d\n",
					  				m_strName.cs(), nMediaIndex, iValue);
		}

		bFound = strParseSemicolonKeyValue(strFmtp, "sprop-parameter-sets", strValue, " ");
		if ( bFound )  {
			const char*		s = strchr(strValue, ',');
			int 			l;

			if ( s )  {
				l = (int)(A(s) - A((const char*)strValue));
				m_nSps = unpackBase64(strValue, l, m_sps, sizeof(m_sps));

				l = (int)strlen(s+1);
				m_nPps = unpackBase64(s+1, l, m_pps, sizeof(m_pps));

				if ( m_nSps == 0 || m_nPps == 0 )  {
					log_warning(L_RTSP, "[rtsp_h264_ch(%s)] wrong SPS/PPS format, parameters are ignored, media %d\n",
									m_strName.cs(), nMediaIndex);
					m_nSps = m_nPps = 0;
				}
				else {
		//log_debug(L_GEN, "[rtsp_h264_ch(%s)] found out-of-band SPROP-PARAMETERS-SETS: '%s'\n", m_strName.cs(), strValue.cs());
		//log_dump_bin(m_sps, m_nSps, "SPS Data: ");
		//log_dump_bin(m_pps, m_nPps, "PPS Data: ");
					log_debug(/*L_GEN,*/ L_RTSP_FL, "[rtsp_h264_ch(%s)] media %d: detected SPS: %d bytes, PPS: %d bytes\n",
							  		m_strName.cs(), nMediaIndex, m_nSps, m_nPps);
				}
			}
		}
	}

	return TRUE;
}

/*
 * Find an compatible media discription in the SDP object,
 * parse specific H264 parameters and initialise descriptor parameters
 *
 * 		pSdp		SDP object pointer
 *
 * Return: ESUCCESS, ...
 */
result_t CRtspChannelH264::initChannel(const CSdp* pSdp)
{
	size_t				i, j, count = pSdp->getMediaCount();
	result_t			nresult = EINVAL, nr;

	CString				strMedia, strProto;
	int 				nPort, numPorts, nProfile;
	std::vector<int>	arProfiles;

	shell_assert(m_nMediaIndex == RTSP_MEDIA_INDEX_UNDEF);

	for(i=0; i<count; i++)  {
		nr = pSdp->getMedia((int)i, strMedia, nPort, numPorts, strProto, arProfiles);

		//log_debug(L_GEN, "parsed media #%d: media: '%s', port %d/%d, proto: '%s', profiles: %d\n",
		//		  (int)i, (const char*)strMedia, nPort, numPorts, (const char*)strProto, arProfiles[0]);

		if ( nr == ESUCCESS )  {
			if ( strMedia != RTSP_H264_CHANNEL_MEDIA_TYPE ) {
				continue;
			}
			if ( strProto != RTSP_H264_CHANNEL_PROTO_TYPE )  {
				continue;
			}

			nProfile = -1;
			for(j=0; j<arProfiles.size(); j++)  {
				if ( arProfiles[j] == RTSP_H264_CHANNEL_PROFILE ) {
					nProfile = arProfiles[j];
					break;
				}
			}

			if ( nProfile < 0 )  {
				continue;
			}

			/*
			 * Check media parameters
			 */
			/* Found appropriate media */
			if ( !getMediaParams((int)i, nProfile, pSdp) )  {
				continue;
			}

			m_nMediaIndex = (int)i;
			nresult = ESUCCESS;
			break;
		}
	}

	return nresult;
}

/*
 * Build 'Transport' header for the SETUP RTSP request
 *
 * 		strBuf			output buffer
 * 		nSize			maximum buffer size, bytes
 */
void CRtspChannelH264::getSetupRequest(char* strBuf, size_t nSize, const CSdp* pSdp)
{
	CString		strMedia, strProto;
	int 		nPort, numPorts;
	std::vector<int> arProfiles;
	result_t	nresult;

	shell_assert(isEnabled());
	shell_assert(m_nMediaIndex >= 0);

	copyString(strBuf, "X-Error: media-error", nSize);

	nresult = pSdp->getMedia(m_nMediaIndex, strMedia, nPort, numPorts, strProto, arProfiles);
	if ( nresult == ESUCCESS )  {
		/*
		 * Find port:
		 * 		1) User supplied port number
		 * 		2) RTSP server supplied port
		 * 		3) Randon even port 16384..32766
		 */
		if ( m_nRtpPort != 0 )  {
			/* User supplied port */
			m_nActualRtpPort = m_nRtpPort;
		}
		else {
			if ( nPort != 0 )  {
				/* RTSP server supplied port */
				m_nActualRtpPort = (ip_port_t)nPort;
			}
			else {
				/* Generate random event port number */
				m_nActualRtpPort = (ip_port_t) ((((int)random())%(32764-16384))+16384);
				m_nActualRtpPort &= 0xfffffffe;

				log_debug(L_RTSP, "[rtsp_h264_ch(%s)] media %d: using random RTP port %u\n",
						  	m_strName.cs(), m_nMediaIndex, m_nActualRtpPort);
			}
		}

		_tsnprintf(strBuf, nSize, "%s: %s;%s;client_port=%u-%u",
				 RTSP_TRANSPORT_HEADER, strProto.cs(), RTSP_H264_CHANNEL_CAST,
				 m_nActualRtpPort, m_nActualRtpPort+1);
	}
}

/*
 * Parse and check server selected transport parameters:
 * 		Transport: <transport>;<unicast>;<client_port=nnnn-nnnn>
 *
 * 		strResponse		response string ('Transport' header)
 * 		pSdp			SDP objects
 *
 * Return:
 * 		ESUCCESS		success
 * 		EINVAL			some parameters are invalid
 */
result_t CRtspChannelH264::checkSetupResponse(const char* strResponse, const CSdp* pSdp)
{
	str_vector_t	strVec;
	size_t			count;
	result_t		nresult;
	boolean_t		bresult;

	CString			strMedia, strProto;
	int 			nPort, numPorts, n, n1, n2;
	std::vector<int> arProfiles;

	CString			strValue;

	nresult = CRtspChannel::checkSetupResponse(strResponse, pSdp);
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	nresult = pSdp->getMedia(m_nMediaIndex, strMedia, nPort, numPorts, strProto, arProfiles);
	if ( nresult != ESUCCESS ) {
		return nresult;
	}

	count = strSplit(strResponse, ';', &strVec, " ");
	if ( count < 2 )  {
		log_debug(L_RTSP, "[rtsp_h264_ch(%s)] media %d: invalid response, count=%d\n",
				  	m_strName.cs(), m_nMediaIndex, count);
		return EINVAL;
	}

	/* Check 'RTP/AVP' */
	if ( strVec[0] != strProto )  {
		log_debug(L_RTSP, "[rtsp_h264_ch(%s)] media %d: unexpected response proto '%s', expected %s\n",
				  	m_strName.cs(), m_nMediaIndex, strVec[0].cs(), strProto.cs());
		return EINVAL;
	}

	/* Check 'unicast' */
	if ( strVec[1] != RTSP_H264_CHANNEL_CAST )  {
		log_debug(L_RTSP, "[rtsp_h264_ch(%s)] media %d: unexpected response cast '%s', expected %s\n",
				  	m_strName.cs(), m_nMediaIndex, strVec[1].cs(), RTSP_H264_CHANNEL_CAST);
		return EINVAL;
	}

	/* Check client_port=n1-n2 */
	shell_assert(m_nActualRtpPort != 0);

	bresult = strParseSemicolonKeyValue(strResponse, "client_port", strValue, " ");
	if ( !bresult )  {
		log_debug(L_RTSP, "[rtsp_h264_desc(%s)] media %d: client_port param is not found\n",
				  	m_strName.cs(), m_nMediaIndex);
		return EINVAL;
	}

	n = _tsscanf(strValue.cs(), "%d-%d", &n1, &n2);
	if ( n != 2 )  {
		log_debug(L_RTSP, "[rtsp_h264_desc(%s)] media %d: client port invalid format '%s'\n",
				  	m_strName.cs(), m_nMediaIndex, strValue.cs());
		return EINVAL;
	}

	if ( n1 != m_nActualRtpPort )  {
		log_debug(L_RTSP, "[rtsp_h264_ch(%s)] media %d: client port %d, request port %d\n",
				  	m_strName.cs(), m_nMediaIndex, n1, m_nActualRtpPort);
		return EINVAL;
	}

	/* Get server_port=n1-n2 */
	bresult = strParseSemicolonKeyValue(strResponse, "server_port", strValue, " ");
	if ( !bresult )  {
		log_debug(L_RTSP, "[rtsp_h264_ch(%s)] media %d: server_port param is not found\n",
				  	m_strName.cs(), m_nMediaIndex);
		return EINVAL;
	}

	n = _tsscanf(strValue.cs(), "%d-%d", &n1, &n2);
	if ( n != 2 )  {
		log_debug(L_RTSP, "[rtsp_h264_ch(%s)] media %d: server_port invalid format '%s'\n",
				  	m_strName.cs(), m_nMediaIndex, strValue.cs());
		return EINVAL;
	}

	if ( n2 != (n1+1) )  {
		log_debug(L_RTSP, "[rtsp_h264_ch(%s)] media %d: server_port invalid ports '%s'\n",
				  	m_strName.cs(), m_nMediaIndex, strValue.cs());
		return EINVAL;
	}
	m_nServerRtpPort = (ip_port_t)n1;

	return ESUCCESS;
}
/*
 * Overrides CRtspChannel::enableRtp()
 */
result_t CRtspChannelH264::enableRtp()
{
	result_t	nresult;

	nresult = CRtspChannel::enableRtp();
	if ( nresult == ESUCCESS )  {
		CRtpPlayoutBufferH264*	pPlayoutBuffer;

		pPlayoutBuffer = dynamic_cast<CRtpPlayoutBufferH264*>(m_pPlayoutBuffer);
		shell_assert(pPlayoutBuffer);

		if ( m_nSps > 0 ) {
			pPlayoutBuffer->setSps(m_sps, m_nSps);
		}
		if ( m_nPps > 0 )  {
			pPlayoutBuffer->setPps(m_pps, m_nPps);
		}
	}

	return nresult;
}

/*******************************************************************************
 * Debugging support
 */

void CRtspChannelH264::dump(const char* strPref) const
{
	CRtspChannel::dump(strPref);

	if ( m_nSps > 0 || m_nPps > 0 )  {
		if ( m_nSps > 0 ) {
			log_dump_bin(m_sps, m_nSps, "    H264 SPS (%d bytes):", m_nSps);
		}
		if ( m_nPps > 0 )  {
			log_dump_bin(m_pps, m_nPps, "    H264 PPS (%d bytes):", m_nPps);
		}
	}

	//m_pInputQueue->dump("*** ");
	m_pPlayoutBuffer->dump();
}