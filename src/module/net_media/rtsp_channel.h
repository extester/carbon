/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTSP protocol media channel base class
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 15.10.2016 16:58:57
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTSP_CHANNEL_H_INCLUDED__
#define __NET_MEDIA_RTSP_CHANNEL_H_INCLUDED__

#include "carbon/carbon.h"

#include "net_media/rtp_input_queue.h"
#include "net_media/sdp.h"
#include "net_media/rtp_session.h"
#include "net_media/media_sink.h"

#define RTSP_MEDIA_INDEX_UNDEF			(-1)

class CRtspChannel
{
	protected:
		const uint32_t		m_id;				/* Channel Id */
		const CString		m_strName;			/* Channel name */
		const ip_port_t		m_nRtpPort;			/* User supplied RTP port, 0 - autoselect */

		boolean_t			m_bEnabled;			/* TRUE: channel enabled, FALSE: disabled */

		int 				m_nMediaIndex;		/* Media index */
		CString				m_strUrl;			/* Channel specific URL (to be added to the base URL) */
		ip_port_t			m_nActualRtpPort;	/* Real RTP port */
		ip_port_t			m_nServerRtpPort;	/* RTP Server port */

		uint32_t			m_nServerSsrc;		/* Server synchronisation source */

		CRtpPlayoutBuffer*	m_pPlayoutBuffer;	/* Playout buffer */
		CVideoSink*			m_pSink;			/* Media channel post processor object */

	public:
		CRtspChannel(uint32_t id, ip_port_t nRtpPort, CRtpPlayoutBuffer* pPlayoutBuffer,
					 CVideoSink* pSink, const char* strName);
		virtual ~CRtspChannel();

	public:
		virtual void reset();
		void setSessionManager(CRtpSessionManager* pSessionMan);

		boolean_t isEnabled() const { return m_bEnabled; }
		boolean_t enable(boolean_t bEnable = TRUE) {
			boolean_t	bPrev = m_bEnabled;
			m_bEnabled = bEnable;
			return bPrev;
		}

		uint32_t getId() const { return m_id; }
		int getMediaIndex() const { return m_nMediaIndex; }
		const char* getUrl() const { return m_strUrl; }
		ip_port_t getRtpPort() const { return m_nActualRtpPort; }
		ip_port_t getRtpServerPort() const { return m_nServerRtpPort; }

		uint32_t getServerSsrc() const { return m_nServerSsrc; }
		CRtpPlayoutBuffer* getPlayoutBuffer() { return m_pPlayoutBuffer; }

		virtual result_t enableRtp();
		virtual void disableRtp();

		virtual void getSetupRequest(char* strBuf, size_t nSize, const CSdp* pSdp) = 0;
		virtual result_t checkSetupResponse(const char* strResponse, const CSdp* pSdp) = 0;
		virtual result_t initChannel(const CSdp* pSdp) = 0;

		void getFrameStat(int* pnNodeDropped) const
		{
			*pnNodeDropped = 0;
			if ( m_pPlayoutBuffer ) {
				m_pPlayoutBuffer->getFrameStat(pnNodeDropped);
			}
		}

		virtual void dump(const char* strPref = "") const;
};

#endif /* __NET_MEDIA_RTSP_CHANNEL_H_INCLUDED__ */
