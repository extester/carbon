/*
 *	Carbon/Network MultiMedia Streaming Module
 *	Media Stream Client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 07.10.2016 12:26:10
 *	    Initial revision.
 */
/*
 * 			+-------------------+
 *			| CRtspClient		|
 * 			+-------------------+
 *
 * 			+-------------------+
 *			| CRtcpClient		|
 * 			+-------------------+
 *
 * 			+-----------------------+
 * 			| CRtpReceiverPool		|
 * 			+-----------------------+
 *
 *
 * 			+-----------------------+		+-----------------------+		+---------------+
 * 			| CRtspChannel			|-----> | CRtpPlayoutBuffer		|----->	| CVideoSink	|
 * 			+-----------------------+		|						|		+---------------+
 * 											|	CRtpInputQueue		|
 * 			...								+-----------------------+
 *
 * 			+-----------------------+		+-----------------------+		+---------------+
 * 			| CRtspChannel			|-----> | CRtpPlayoutBuffer		|----->	| CVideoSink	|
 * 			+-----------------------+		|						|		+---------------+
 * 											|	CRtpInputQueue		|
 * 											+-----------------------+
 *
 */

#ifndef __NET_MEDIA_MEDIA_CLIENT_H_INCLUDED__
#define __NET_MEDIA_MEDIA_CLIENT_H_INCLUDED__

#include <vector>

#include "shell/netaddr.h"
#include "carbon/carbon.h"
#include "carbon/fsm.h"

#include "net_media/sdp.h"
#include "net_media/rtsp_client.h"
#include "net_media/rtp_receiver_pool.h"
#include "net_media/rtsp_channel.h"
#include "net_media/rtp_session.h"
#include "net_media/rtcp_client.h"

#define NET_MEDIA_LIBRARY_USER_AGENT			"NetMedia Library"

class CMediaClient;

/*
 * Interface: media client => application
 */
class IMediaClient
{
	public:
		virtual void onMediaClientConnect(CMediaClient* pClient, result_t nresult) = 0;
		virtual void onMediaClientConfigure(CMediaClient* pClient, result_t nresult) = 0;
		virtual void onMediaClientPlay(CMediaClient* pClient, result_t nresult) = 0;
		virtual void onMediaClientPause(CMediaClient* pClient, result_t nresult) = 0;
		virtual void onMediaClientDisconnect(CMediaClient* pClient, result_t nresult) = 0;
};

class CMediaClient : public CModule, public CStateMachine, public IRtspClient
{
	protected:
		CEventLoop*				m_pOwnerLoop;			/* Event loop belong to */
		IMediaClient*			m_pParent;				/* Notification object */
		const CNetHost			m_selfHost;				/* Self ip address, required */
		CNetHost				m_serverAddr;			/* RTSP/RTP/RTCP server ip */

		std::vector<CRtspChannel*>	m_arChannel;		/* Channels */
		CRtspClient				m_rtsp;					/* RTSP protocol client */
		CRtcpClient				m_rtcp;					/* RTCP protocol client */

		int						m_asyncChannelIndex;
		result_t				m_asyncResult;

		CRtpReceiverPool*		m_pReceiverPool;		/* RTP frame receiver */
		CRtpSessionManager		m_sessionMan;

	public:
		CMediaClient(const CNetHost& selfHost, CEventLoop* pOwnerLoop,
					 	IMediaClient* pParent, const char* strName);
		virtual ~CMediaClient();

	public:
		virtual result_t init();
		virtual void terminate();
		void reset();

		result_t insertChannel(CRtspChannel* pChannel);
		void removeChannel(CRtspChannel* pChannel);

		void connect(const CNetAddr& rtspServerAddr, const char* strServerUrl);
		void configure();
		void play();
		void pause();
		void disconnect();

		void getReceiverStat(const CRtpPlayoutBuffer* pBuffer, int* pnFrameLost) const
		{
			m_pReceiverPool->getStat(pBuffer, pnFrameLost);
		}
		void dump(const char* strPref = "") const;

	private:
		void disableRtpChannels();
		void configureNext();
		uint32_t getServerSsrc() const;

		result_t enableRtp();
		void disableRtp();

		result_t enableRtcp();
		void disableRtcp();

		/* RTSP result interface implementation (RTSP client => net media client) */
		virtual void onRtspConnect(CRtspClient* pRtspClient, result_t nresult);
		virtual void onRtspConfigure(CRtspClient* pRtspClient, result_t nresult);
		virtual void onRtspPlay(CRtspClient* pRtspClient, result_t nresult);
		virtual void onRtspPause(CRtspClient* pRtspClient, result_t nresult);
		virtual void onRtspDisconnect(CRtspClient* pRtspClient, result_t nresult);
};

#endif /* __NET_MEDIA_MEDIA_CLIENT_H_INCLUDED__ */
