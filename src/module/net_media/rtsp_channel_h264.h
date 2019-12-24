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
 *	Revision 1.0, 25.10.2016 17:30:14
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTSP_CHANNEL_H264_H_INCLUDED__
#define __NET_MEDIA_RTSP_CHANNEL_H264_H_INCLUDED__

#include "net_media/rtp_playout_buffer_h264.h"
#include "net_media/rtsp_channel.h"

class CRtspChannelH264 : public CRtspChannel
{
	protected:
		uint8_t			m_sps[1024];		/* H264 SPS data (if m_nSps > 0) */
		size_t			m_nSps;				/* H264 SPS data length, bytes (may be 0) */
		uint8_t			m_pps[1024];		/* H264 PPS data (if m_nPps > 0) */
		size_t			m_nPps;				/* H264 PPS data length, bytes (may be 0) */

	public:
		CRtspChannelH264(uint32_t id, ip_port_t nRtpPort, int nFps, int nRtpMaxDelay,
							CVideoSink* pSink, const char* strName);
		virtual ~CRtspChannelH264();

	public:
		virtual void reset();
		virtual result_t initChannel(const CSdp* pSdp);
		virtual void getSetupRequest(char* strBuf, size_t nSize, const CSdp* pSdp);
		virtual result_t checkSetupResponse(const char* strResponse, const CSdp* pSdp);

		virtual result_t enableRtp();

		virtual void dump(const char* strPref = "") const;

	private:
		boolean_t getMediaParams(int nMediaIndex, int nProfile, const CSdp* pSdp);
		size_t unpackBase64(const char* pSrc, size_t nSrcLength, uint8_t* pDst, size_t nDstMax);

};

#endif /* __NET_MEDIA_RTSP_CHANNEL_H264_H_INCLUDED__ */
