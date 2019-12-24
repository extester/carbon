/*
 *	Carbon/Network MultiMedia Streaming Module
 *	Class Encapsulates Single Video H264 media channel
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 02.11.2016 17:02:56
 *		Initial revision.
 */

#ifndef __RTP_VIDEO_H264_H_INCLUDED__
#define __RTP_VIDEO_H264_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/cstring.h"
#include "carbon/timer.h"
#include "carbon/fsm.h"

#include "net_media/rtsp_channel_h264.h"
#include "net_media/media_client.h"
#include "net_media/media_sink.h"

class CRtpVideoH264;

class IRtpVideoH264Result
{
	public:
		virtual void onRtpVideoH264Stop(CRtpVideoH264* pChannel) = 0;
};

class CRtpVideoH264 : public CStateMachine, public IMediaClient
{
	private:
		char					m_strName[64];		/* Camera name */
		IRtpVideoH264Result*	m_pParent;			/* Notify object */
		CEventLoop*				m_pEventLoop;		/* Own event loop */

		CNetHost				m_rtspHost;			/* RTSP server IP */
		CString					m_strRtspUrl;		/* RTSP URL */
		ip_port_t				m_nRtpPort;			/* User RTP port (0-auto) */

		CVideoSink*				m_pSink;			/* RTP Video processor or 0 */
		CRtspChannel*			m_pChannel;			/* Media channel object or 0 */
		CMediaClient*			m_pClient;			/* Media client or 0 */

		int 					m_nInterval;		/* Restart interval, seconds */
		CTimer*					m_pTimer;			/* Restart timer */

	public:
		CRtpVideoH264(uint32_t id, const CNetAddr& selfAddr, const CNetHost& rtspHost,
					 	const char* rtspUrl, ip_port_t nRtpPort, int nFps, int nRtpMaxDelay,
					  	CVideoSink* pSink, IRtpVideoH264Result* pParent, const char* strName,
					  	int nInterval = 8, CEventLoop* pEventLoop = 0);
		virtual ~CRtpVideoH264();

	public:
		void start();
		void stop();
		boolean_t isStopped() const;

		void getFrameStat(int* pnReceiverFrameLost, int* pnNodeDropped) const
		{
			*pnReceiverFrameLost = *pnNodeDropped = 0;
			if ( m_pChannel )  {
				m_pChannel->getFrameStat(pnNodeDropped);

				if ( m_pClient ) {
					m_pClient->getReceiverStat(m_pChannel->getPlayoutBuffer(),
											   pnReceiverFrameLost);
				}
			}
		}

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual void onMediaClientConnect(CMediaClient* pClient, result_t nresult);
		virtual void onMediaClientConfigure(CMediaClient* pClient, result_t nresult);
		virtual void onMediaClientPlay(CMediaClient* pClient, result_t nresult);
		virtual void onMediaClientPause(CMediaClient* pClient, result_t nresult);
		virtual void onMediaClientDisconnect(CMediaClient* pClient, result_t nresult);

	private:
		void restart();
		void stopTimer();
		void restartTimer();
		void timerHandler(void* p);
};

#endif /* __RTP_VIDEO_H264_H_INCLUDED__ */
