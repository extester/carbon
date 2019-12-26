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
 *	Revision 1.0, 02.11.2016 13:17:28
 *		Initial revision.
 */

#include "net_media/rtp_video_h264.h"

enum {
	VIDEO_FSM_NONE				= 0,
	VIDEO_FSM_CONNECTING		= 1,
	VIDEO_FSM_CONFIGURING		= 2,
	VIDEO_FSM_PLAY				= 3,
	VIDEO_FSM_DISCONNECTING		= 4,
	VIDEO_FSM_RESTART			= 5
};

/*******************************************************************************
 * CRtpVideoH264 class
 */

CRtpVideoH264::CRtpVideoH264(uint32_t id, const CNetAddr& selfAddr,
							   	const CNetHost& rtspHost, const char* rtspUrl,
							 	ip_port_t nRtpPort, int nFps, int nRtpMaxDelay,
							   	CVideoSink* pSink, IRtpVideoH264Result* pParent,
							   	const char* strName, int nInterval, CEventLoop* pEventLoop) :
	CStateMachine(VIDEO_FSM_NONE),
	IMediaClient(),
	m_pParent(pParent),
	m_pEventLoop(pEventLoop),
	m_rtspHost(rtspHost),
	m_strRtspUrl(rtspUrl),
	m_nRtpPort(nRtpPort),
	m_pSink(pSink),
	m_nInterval(nInterval),
	m_pTimer(0)
{
	shell_assert(pParent);
	shell_assert(strName);

	copyString(m_strName, strName, sizeof(m_strName));

	if ( pEventLoop == 0 ) {
		m_pEventLoop = appMainLoop();
	}

	m_pChannel = new CRtspChannelH264(id, nRtpPort, nFps, nRtpMaxDelay, pSink, strName);
	m_pClient = new CMediaClient(selfAddr, m_pEventLoop, this, strName);
	m_pClient->insertChannel(m_pChannel);
}

CRtpVideoH264::~CRtpVideoH264()
{
	shell_assert(m_pClient);
	if ( m_pClient )  {
		m_pClient->removeChannel(m_pChannel);
		SAFE_DELETE(m_pClient);
	}

	SAFE_DELETE(m_pChannel);
}

boolean_t CRtpVideoH264::isStopped() const
{
	return getFsmState() == VIDEO_FSM_NONE;
}



void CRtpVideoH264::stopTimer()
{
	SAFE_DELETE_TIMER(m_pTimer, m_pEventLoop);
}

/*
 * Restart camera starting up timer
 */
void CRtpVideoH264::restartTimer()
{
	char 	strTemp[128];

	stopTimer();

	_tsnprintf(strTemp, sizeof(strTemp), "%s-restart", m_strName);
	m_pTimer = new CTimer(SECONDS_TO_HR_TIME(m_nInterval),
						  TIMER_CALLBACK(CRtpVideoH264::timerHandler, this),
						  0, strTemp);
	m_pEventLoop->insertTimer(m_pTimer);
}

/*
 * Restart timer handler: try to initialise camera
 */
void CRtpVideoH264::timerHandler(void* p)
{
	fsm_t	fsm = getFsmState();

	shell_assert_ex(fsm == VIDEO_FSM_RESTART, "state=%d", fsm);

	m_pTimer = 0;
	if ( fsm == VIDEO_FSM_RESTART ) {
		start();
	}
}

/*
 * Restart camera iniitalisation on any error
 */
void CRtpVideoH264::restart()
{
	log_trace(L_RTP_VIDEO_H264, "[video(%s)] restarting...\n", m_strName);

	setFsmState(VIDEO_FSM_RESTART);
	m_pClient->disconnect();
}

void CRtpVideoH264::onMediaClientConnect(CMediaClient* pClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == VIDEO_FSM_CONNECTING ) {
		char 	strBuf[128];

		if ( nresult == ESUCCESS )  {
			copyString(strBuf, "connected successful", sizeof(strBuf));
		}
		else {
			_tsnprintf(strBuf, sizeof(strBuf), "connection failure, result %d", nresult);
		}

		log_trace(L_RTP_VIDEO_H264, "[video(%s)] %s\n", m_strName, strBuf);

		if ( nresult == ESUCCESS ) {
			setFsmState(VIDEO_FSM_CONFIGURING);
			m_pClient->configure();
		}
		else {
			restart();
		}
	}
	else {
		log_debug(L_RTP_VIDEO_H264, "[video(%s)] fsm missing (%d/%d)\n", m_strName, fsm, nresult);
	}
}

void CRtpVideoH264::onMediaClientConfigure(CMediaClient* pClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == VIDEO_FSM_CONFIGURING ) {
		log_trace(L_RTP_VIDEO_H264, "[video(%s)] configure result %d\n", m_strName, nresult);

		if ( nresult == ESUCCESS ) {
			setFsmState(VIDEO_FSM_PLAY);
			m_pClient->play();
		}
		else {
			restart();
		}
	}
	else {
		log_debug(L_RTP_VIDEO_H264, "[video(%s)] fsm missing (%d/%d)\n", m_strName, fsm, nresult);
	}
}

void CRtpVideoH264::onMediaClientPlay(CMediaClient* pClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == VIDEO_FSM_PLAY ) {
		log_trace(L_RTP_VIDEO_H264, "[video(%s)] play result %d\n", m_strName, nresult);

		if ( nresult == ESUCCESS ) {
			log_trace(L_RTP_VIDEO_H264, "[video(%s)] -- streaming --\n", m_strName);
		}
		else {
			restart();
		}
	}
	else {
		log_debug(L_RTP_VIDEO_H264, "[video(%s)] fsm missing (%d/%d)\n", m_strName, fsm, nresult);
	}
}

void CRtpVideoH264::onMediaClientPause(CMediaClient* pClient, result_t nresult)
{
	UNUSED(pClient);
	UNUSED(nresult);
	shell_assert(FALSE);
}

void CRtpVideoH264::onMediaClientDisconnect(CMediaClient* pClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	log_trace(L_RTP_VIDEO_H264, "[video(%s)] disconnected, result %d\n", m_strName, nresult);

	shell_assert(m_pClient);

	m_pClient->terminate();

	if ( fsm != VIDEO_FSM_RESTART )  {
		setFsmState(VIDEO_FSM_NONE);
		m_pParent->onRtpVideoH264Stop(this);
	}
	else {
		restartTimer();
	}
}

/*
 * Start camera streaming
 */
void CRtpVideoH264::start()
{
	fsm_t		fsm = getFsmState();
	CNetAddr	rtspAddr;
	result_t	nresult;

	if ( fsm == VIDEO_FSM_DISCONNECTING )  {
		setFsmState(VIDEO_FSM_RESTART);
		return;
	}

	stopTimer();
	setFsmState(VIDEO_FSM_NONE);

	if ( m_rtspHost.isValid() )  {
		rtspAddr = CNetAddr(m_rtspHost, RTSP_PORT);
	}

	nresult = m_pClient->init();
	if ( nresult == ESUCCESS )  {
		setFsmState(VIDEO_FSM_CONNECTING);
		m_pClient->connect(rtspAddr, m_strRtspUrl);
	}
	else {
		log_error(L_RTP_VIDEO_H264, "[video(%s)] init failure, result %d\n", m_strName, nresult);
		setFsmState(VIDEO_FSM_RESTART);
		restartTimer();
	}
}

/*
 * Stop camera streaming
 *
 * On stop IRtpVideoH264Result::onRtpVideoH264Stop() is called
 */
void CRtpVideoH264::stop()
{
	fsm_t	fsm = getFsmState();

	if ( fsm == VIDEO_FSM_NONE )  {
		m_pParent->onRtpVideoH264Stop(this);
	}
	else {
		stopTimer();
		setFsmState(VIDEO_FSM_DISCONNECTING);
		m_pClient->disconnect();
	}
}

/*******************************************************************************
 * Debugging support
 */

void CRtpVideoH264::dump(const char* strPref) const
{
	log_dump("*** %sVideoH264: %s, fsm: %d, url: %s, rtp port: %d, restart timer: %s\n",
			 strPref, m_strName, getFsmState(), m_strRtspUrl.cs(),
			 m_nRtpPort, m_pTimer ? "ON" : "OFF");

	if ( m_pClient )  {
		m_pClient->dump();
	}
}
