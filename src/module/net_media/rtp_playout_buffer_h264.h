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
 *	Revision 1.0, 17.10.2016 13:31:19
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTP_PLAYOUT_BUFFER_H264_H_INCLUDED__
#define __NET_MEDIA_RTP_PLAYOUT_BUFFER_H264_H_INCLUDED__

#include "net_media/h264.h"
#include "net_media/rtp_playout_buffer.h"

class CRtpPlayoutNodeH264 : public CRtpPlayoutNode
{
	enum {
		flagLast		= 1,		/* Node contains a last frame (marker=1) */
		flagParams		= 2,		/* Node contains a SPS/PPS frames */
		flagIdr			= 4,		/* Node contains IDR frame (key frame) */
		flagReady		= 8			/* Node has been built and ready for playout */
	};

	protected:
		int 			m_flags;		/* Various flags, flagXXX */
		uint8_t*		m_pData;		/* Compressed H264 frame data or NULL */
		size_t			m_nSize;		/* Compressed data size, bytes */

	public:
		CRtpPlayoutNodeH264(uint64_t rtpRealTimestamp, hr_time_t hrPlayoutTime,
							CRtpPlayoutBuffer* pParent);
		virtual ~CRtpPlayoutNodeH264();

	public:
		virtual result_t insertFrame(rtp_frame_t* pFrame);
		virtual boolean_t isReady() const { return (m_flags&flagReady) != 0; }

		virtual uint8_t* getData() { return m_pData; }
		virtual size_t getSize() const { return m_nSize; }
		virtual boolean_t isIdrFrame() const { return (m_flags&flagIdr) != 0; }

		virtual void dump(const char* strPref = "") const;
		virtual void dumpFrames(const char* strMargin = "") const;
		virtual void dumpFramePayload(int index, const rtp_frame_t* pFrame, const char* strMargin) const;

		int dumpPayloadFua(const uint8_t* pPayload, size_t payloadLen, char* strBuf, int strBufLen) const;
		int dumpPayloadSingle(const uint8_t* pPayload, size_t payloadLen, char* strBuf, int strBufLen) const;

	private:
		boolean_t checkNodeValid() const;
		result_t buildCompressed();
};

class CRtpPlayoutBufferH264 : public CRtpPlayoutBuffer
{
	protected:
		uint8_t		m_Sps[1024];
		size_t		m_nSps;
		uint8_t		m_Pps[1024];
		size_t 		m_nPps;

		counter_t	m_nUnsupportedFrames;	/* DBG: unsupported RTP frames */

	public:
		CRtpPlayoutBufferH264(int nProfile, int nFps, int nClockRate, int nMaxInputQueue,
							  	int nMaxDelay, const char* strName);
		virtual ~CRtpPlayoutBufferH264();

	public:
		virtual result_t init(CVideoSink* pSink);
		virtual void terminate();

		void setSps(void* pSps, size_t size);
		void setPps(void* pPps, size_t size);
		size_t getSPdata(void* pBuffer) const;
		size_t getSPsize() const;

		void incrUnsupportedFrame() { counter_inc(m_nUnsupportedFrames); }

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual CRtpPlayoutNode* createNode(rtp_frame_t* pFrame, uint64_t rtpRealTimestamp);

	private:
		boolean_t validateFrame(const rtp_frame_t* pFrame) const;

};

#endif /* __NET_MEDIA_RTP_PLAYOUT_BUFFER_H264_H_INCLUDED__ */
