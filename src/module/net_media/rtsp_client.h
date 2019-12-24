/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTSP protocol client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 03.10.2016 15:18:29
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTSP_CLIENT_H_INCLUDED__
#define __NET_MEDIA_RTSP_CLIENT_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/fsm.h"
#include "carbon/timer.h"
#include "carbon/http_container.h"
#include "carbon/net_server/net_client.h"

#include "net_media/sdp.h"
#include "net_media/rtsp_channel.h"

#define RTSP_PORT						554

#define RTSP_CSEQ_HEADER				"CSeq"
#define RTSP_SESSION_HEADER				"Session"
#define RTSP_TRANSPORT_HEADER			"Transport"
#define RTSP_USER_AGENT_HEADER			"User-Agent"
#define RTSP_SERVER_HEADER				"Server"

#define RTSP_CONTENT_TYPE_HEADER		"Content-Type"
#define RTSP_CONTENT_BASE_HEADER		"Content-Base"
#define RTSP_CONTENT_LOCATION_HEADER	"Content-Location"
#define RTSP_REQUEST_URI_HEADER			"request-uri"
#define RTSP_RANGE_HEADER				"Range"

typedef uint32_t	rtsp_seqnum_t;

class CRtspClient;

class IRtspClient
{
	public:
		virtual void onRtspConnect(CRtspClient* pRtspClient, result_t nresult) = 0;
		virtual void onRtspConfigure(CRtspClient* pRtspClient, result_t nresult) = 0;
		virtual void onRtspPlay(CRtspClient* pRtsp, result_t nresult) = 0;
		virtual void onRtspPause(CRtspClient* pRtsp, result_t nresult) = 0;
		virtual void onRtspDisconnect(CRtspClient* pRtspClient, result_t nresult) = 0;
};

enum {
	RTSP_FSM_NONE,				/* Uninitialised state */
	RTSP_FSM_CONNECTED,			/* Initialised state, CONNECT/OPTIONS/DESCRIBE completed */
	RTSP_FSM_CONFIGURED,		/* Configured state, ready for PLAY/PAUSE/TEARDOWN */
	RTSP_FSM_PLAYBACK,			/* Playing */

	RTSP_FSM_CONNECT,			/* Connecting: connecting */
	RTSP_FSM_OPTIONS,			/* Connecting: performing OPTIONS */
	RTSP_FSM_DESCRIBE,			/* Connecting: performing DESCRIBE */
	RTSP_FSM_SETUP,				/* Configuring: performing SETUP */

	RTSP_FSM_PLAYING,			/* Performing PLAY */
	RTSP_FSM_PAUSING,			/* Performing PAUSE */
	RTSP_FSM_TEARDOWN			/* Performing TEARDOWN */
};

#define RTSP_OPTION_DESCRIBE			0x0001	/* Recommended */
#define RTSP_OPTION_ANNOUNCE			0x0002	/* Optional */
#define RTSP_OPTION_GET_PARAMETER		0x0004	/* Optional */
#define RTSP_OPTION_OPTIONS				0x0008	/* Required, server=>client: optional */
#define RTSP_OPTION_PAUSE				0x0010	/* Recommended */
#define RTSP_OPTION_PLAY				0x0020	/* Required */
#define RTSP_OPTION_RECORD				0x0040	/* Optional */
#define RTSP_OPTION_REDIRECT			0x0080	/* Optional */
#define RTSP_OPTION_SETUP				0x0100	/* Required */
#define RTSP_OPTION_SET_PARAMETER		0x0200	/* Optional */
#define RTSP_OPTION_TEARDOWN			0x0400	/* Required */


class CRtspClient : public CModule, public CEventReceiver, public CStateMachine
{
	protected:
		CEventLoop*			m_pOwnerLoop;		/* Owner event loop */
		IRtspClient*		m_pParent;			/* Notification obejct */

		CNetClient			m_netClient;		/* Network I/O client */
		CNetHost			m_selfHost;			/* Bind address */
		CNetAddr			m_rtspServerAddr;	/* Remote RTSP server address */
		CString				m_rtspServerUrl;	/* Remote RTSP server URL */
		rtsp_seqnum_t		m_rtspSeq;			/* Next sequnce number */
		seqnum_t			m_sessId;			/* NetClient session Id */

		int 				m_rtspOptions;		/* Protocol capability options */
		CString				m_rtspSession;		/* RTSP session ID or "" */
		CSdp				m_sdp;				/* Media descriptions */
		CString				m_strBaseUrl;		/* RTSP Base URL or "" (available after 'DESCRIBE')*/

		CTimer*				m_pIdleTimer;		/* Idle timeout */
		int 				m_rtspTimeout;		/* Maximum IDLE timeout, seconds */

		CRtspChannel*		m_pAsyncChannel;	/* Current RTSP channel (for configure()) */

	public:
		CRtspClient(const CNetHost& selfHost, CEventLoop* pOwnerLoop, IRtspClient* pParent);
		virtual ~CRtspClient();

	public:
		virtual result_t init();
		virtual void terminate();

		result_t initChannel(CRtspChannel* pChannel) const {
			return pChannel->initChannel(&m_sdp);
		}

		void connect(const CNetAddr& rtspServerAddr, const char* strServerUrl);
		void configure(CRtspChannel* pChannel);
		void play();
		void pause();
		void disconnect();

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);

	private:
		void reset();

		result_t parseSession(const char* strSessionHeader, CString& strSession, int& timeout) const;
		rtsp_seqnum_t getRtspSeq();
		result_t checkRtspSeq(CHttpContainer* pContainer, boolean_t bCheckSess = TRUE) const;
		void buildRequest(const char* strRequest, CHttpContainer* pContainer,
						  const CRtspChannel* pChannel);
		void executeRequest(CHttpContainer* pContainer);

		void stopIdleTimer();
		void startIdleTimer();
		void onIdleTimer(void* p);

		void doDisconnect();
		void doOptions();
		void doDescribe();
		void doPlayback();
		void doPause();
		void doTeardown();

		result_t processOptions(CHttpContainer* pContainer);
		result_t processDescribe(CHttpContainer* pContainer);
		result_t processSetup(CHttpContainer* pContainer);
		result_t processPlay(CHttpContainer* pContainer);
		result_t processPause(CHttpContainer* pContainer);
		result_t processTeardown(CHttpContainer* pContainer);
};

#endif /* __NET_MEDIA_RTSP_CLIENT_H_INCLUDED__ */
