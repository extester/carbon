/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP per session data
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 10.11.2016 18:09:17
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTP_SESSION_H_INCLUDED__
#define __NET_MEDIA_RTP_SESSION_H_INCLUDED__

#include <map>

#include "carbon/lock.h"
#include "carbon/carbon.h"

#include "net_media/rtp.h"
#include "net_media/rtcp.h"


class CRtpSession
{
	protected:
		uint32_t		m_ssrc;
		rtp_source_t*	m_pSource;
		int 			m_rate;

		uint32_t		m_lsr;
		hr_time_t		m_hrLsr;

	public:
		CRtpSession(uint32_t ssrc, rtp_source_t* pSource, int rate);
		virtual ~CRtpSession();

	public:
		void onRtpFrame(const rtp_frame_t* pFrame);
		void onRtcpSrPacket(const rtcp_sr_packet_t* pPacket);
		void getData(rtcp_report_block_t* pBlock);

};

class CRtpSessionManager
{
	protected:
		std::map<uint32_t, CRtpSession*>	m_session;
		mutable CMutex	m_lock;

	public:
		CRtpSessionManager();
		virtual ~CRtpSessionManager();

	public:
		void onRtpFrame(const rtp_frame_t* pFrame, rtp_source_t* pSource, int rate);
		void onRtcpSrPacket(const rtcp_sr_packet_t* pPacket);
		void getSessionData(uint32_t ssrc, rtcp_report_block_t* pBlock);

		void dump(const char* strPref = "") const;

	private:
		CRtpSession* createSession(uint32_t ssrc, rtp_source_t* pSource, int rate);
		void deleteSession(uint32_t ssrc);
};

#endif /* __NET_MEDIA_RTP_SESSION_H_INCLUDED__ */
