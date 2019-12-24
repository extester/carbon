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
 *	Revision 1.0, 03.10.2016 15:50:54
 *		Initial revision.
 */

#include "carbon/utils.h"

#include "net_media/sdp.h"
#include "net_media/rtsp_client.h"
#include "net_media/media_client.h"

#define MODULE_NAME						"RTSP_CLIENT"

#define RTSP_VER						"RTSP/1.0"
#define RTSP_SUCCESS_RESPONSE			"RTSP/1.0 200 OK"

#define RTSP_TIMEOUT_DEFAULT			60 	/* Default network I/O timeout (ping interval), seconds */

#define RTSP_REQUIRED_OPTIONS			(RTSP_OPTION_OPTIONS|RTSP_OPTION_DESCRIBE|\
										RTSP_OPTION_SETUP|RTSP_OPTION_PLAY|\
										RTSP_OPTION_TEARDOWN)

/*******************************************************************************
 * CRtspClient class
 */

CRtspClient::CRtspClient(const CNetHost& selfHost, CEventLoop* pOwnerLoop,
						 IRtspClient* pParent) :
	CModule(MODULE_NAME),
	CEventReceiver(pOwnerLoop, MODULE_NAME),
	CStateMachine(RTSP_FSM_NONE),
	m_pOwnerLoop(pOwnerLoop),
	m_pParent(pParent),
	m_netClient(this),
	m_selfHost(selfHost),

	m_rtspSeq(0),
	m_sessId(NO_SEQNUM),
	m_rtspOptions(0),
	m_pIdleTimer(0),
	m_rtspTimeout(RTSP_TIMEOUT_DEFAULT),
	m_pAsyncChannel(0)
{
}

CRtspClient::~CRtspClient()
{
	shell_assert(m_netClient.isConnected() != ESUCCESS);
	shell_assert(!m_pIdleTimer);
}

/*
 * Parse Session Header:
 * 		Session: <session>[;timeout=n]
 */
result_t CRtspClient::parseSession(const char* strSessionHeader,
								   CString& strSession, int& timeout) const
{
	str_vector_t	vec;
	size_t			n;
	int 			temp;
	result_t		nresult = EINVAL;

	strSession.clear();
	timeout = -1;

	n = strSplit(strSessionHeader, ';', &vec, " ");
	if ( n > 0 )  {
		nresult = ESUCCESS;
		strSession = vec[0];
	}
	if ( n > 1 ) {
		if ( _tmemcmp((const char*)vec[1], "timeout=", 8) == 0 ) {
			temp = _tatoi(vec[1].cs()+8);
			if ( temp > 0 )  {
				timeout = temp;
			}
		}
	}

	return nresult;
}

/*
 * Get net available unique RTSP request id
 *
 * Return: unique Id
 */
rtsp_seqnum_t CRtspClient::getRtspSeq()
{
	while ( (++m_rtspSeq) == 0 );
	return m_rtspSeq;
}

result_t CRtspClient::checkRtspSeq(CHttpContainer* pContainer, boolean_t bCheckSess) const
{
	CString		strCSeq, strSessHead, strSess;
	int 		cseq, timeout;
	result_t	nresult = EINVAL;

	pContainer->getHeader(strCSeq, RTSP_CSEQ_HEADER);
	if ( strCSeq.getNumber(cseq) == ESUCCESS )  {
		if ( cseq == (int)m_rtspSeq )  {
			nresult = ESUCCESS;
			if ( bCheckSess && !m_rtspSession.isEmpty() ) {
				pContainer->getHeader(strSessHead, RTSP_SESSION_HEADER);
				parseSession(strSessHead, strSess, timeout);
				if ( strSess != m_rtspSession ) {
					log_debug(L_RTSP, "[rtsp_cli] wrong session '%s', expected '%s'\n",
							  strSess.cs(), m_rtspSession.cs());
					nresult = EINVAL;
				}
			}
		}
		else {
			log_debug(L_RTSP, "[rtsp_cli] wrong cseq %u, expected %u\n", cseq, m_rtspSeq);
		}
	}
	else {
		log_debug(L_RTSP, "[rtsp_cli] no valid %s header found\n", RTSP_CSEQ_HEADER);
	}

	return nresult;
}

/*
 * Event processor
 *
 *      pEvent      event object to process
 *
 * Return:
 *      TRUE        event processed
 *      FALSE       event is not processed
 */
boolean_t CRtspClient::processEvent(CEvent* pEvent)
{
	CEventNetClientRecv*	pEventRecv;
	CHttpContainer*			pContainer;
	result_t				nresult;
	boolean_t       		bProcessed = FALSE;

	switch ( pEvent->getType() )  {
		case EV_NET_CLIENT_CONNECTED:
			nresult = (result_t)pEvent->getnParam();

			if ( pEvent->getSessId() == m_sessId )  {
				m_sessId = NO_SEQNUM;
				if ( getFsmState() == RTSP_FSM_CONNECT )  {
					if ( nresult == ESUCCESS ) {
						log_debug(L_RTSP_FL, "[rtsp_cli] server %s connected\n",
								  (const char*)m_rtspServerAddr);
						doOptions();
					}
					else {
						setFsmState(RTSP_FSM_NONE);
						m_pParent->onRtspConnect(this, nresult);
					}
				}
			}
			bProcessed = TRUE;
			break;

		case EV_NET_CLIENT_RECV:
			pEventRecv = dynamic_cast<CEventNetClientRecv*>(pEvent);

			if ( pEvent->getSessId() == m_sessId ) {
				m_sessId = NO_SEQNUM;
				nresult  = pEventRecv->getResult();

				switch ( getFsmState() ) {
					case RTSP_FSM_OPTIONS:
						if ( nresult == ESUCCESS ) {
							pContainer = dynamic_cast<CHttpContainer*>(pEventRecv->getContainer());
							shell_assert(pContainer);
							nresult = checkRtspSeq(pContainer);
							if ( nresult  == ESUCCESS ) {
								nresult = processOptions(pContainer);
							}
						}
						if ( nresult != ESUCCESS )  {
							doDisconnect();
							m_pParent->onRtspConnect(this, nresult);
						}
						break;

					case RTSP_FSM_DESCRIBE:
						if ( nresult == ESUCCESS ) {
							pContainer = dynamic_cast<CHttpContainer*>(pEventRecv->getContainer());
							shell_assert(pContainer);
							nresult = checkRtspSeq(pContainer);
							if ( nresult == ESUCCESS ) {
								nresult = processDescribe(pContainer);
							}
						}
						if ( nresult != ESUCCESS )  {
							doDisconnect();
							m_pParent->onRtspConnect(this, nresult);
						}
						break;

					case RTSP_FSM_SETUP:
						if ( nresult == ESUCCESS ) {
							pContainer = dynamic_cast<CHttpContainer*>(pEventRecv->getContainer());
							shell_assert(pContainer);
							nresult = checkRtspSeq(pContainer);
							if ( nresult == ESUCCESS ) {
								nresult = processSetup(pContainer);
							}
						}
						if ( nresult != ESUCCESS )  {
							m_rtspSession.clear();
							m_rtspTimeout = RTSP_TIMEOUT_DEFAULT;
							setFsmState(RTSP_FSM_CONNECTED);
							m_pParent->onRtspConfigure(this, nresult);
						}
						break;

					case RTSP_FSM_PLAYING:
						if ( nresult == ESUCCESS ) {
							pContainer = dynamic_cast<CHttpContainer*>(pEventRecv->getContainer());
							shell_assert(pContainer);
							nresult = checkRtspSeq(pContainer);
							if ( nresult == ESUCCESS ) {
								nresult = processPlay(pContainer);
							}
						}
						if ( nresult != ESUCCESS )  {
							setFsmState(RTSP_FSM_CONFIGURED);
							m_pParent->onRtspPlay(this, nresult);
						}
						break;

					case RTSP_FSM_PAUSING:
						if ( nresult == ESUCCESS ) {
							pContainer = dynamic_cast<CHttpContainer*>(pEventRecv->getContainer());
							shell_assert(pContainer);
							nresult = checkRtspSeq(pContainer, FALSE);
							if ( nresult == ESUCCESS ) {
								nresult = processPause(pContainer);
							}
						}
						if ( nresult != ESUCCESS )  {
							setFsmState(RTSP_FSM_CONFIGURED);
							m_pParent->onRtspPause(this, nresult);
						}
						break;

					case RTSP_FSM_TEARDOWN:
						if ( nresult == ESUCCESS ) {
							pContainer = dynamic_cast<CHttpContainer*>(pEventRecv->getContainer());
							shell_assert(pContainer);
							nresult = checkRtspSeq(pContainer, FALSE);
							if ( nresult == ESUCCESS ) {
								nresult = processTeardown(pContainer);
							}
						}
						if ( nresult != ESUCCESS )  {
							doDisconnect();
							m_pParent->onRtspDisconnect(this, nresult);
						}
						break;
				}
			}
			bProcessed = TRUE;
			break;
	}

	return bProcessed;
}

/*
 * Reset RTSP client to uninitialisaed state
 */
void CRtspClient::reset()
{
	m_rtspServerAddr = CNetAddr();
	m_rtspServerUrl.clear();
	m_rtspOptions = 0;
	m_sdp.clear();
	m_strBaseUrl.clear();
	m_rtspSession.clear();
	m_rtspSeq = 0;

	m_rtspTimeout = RTSP_TIMEOUT_DEFAULT;
	stopIdleTimer();
}

/*
 * Make a complete RTSP request
 *
 * 		strRequest		request ("DESCRIBE", "OPTIONS", ...)
 * 		pContainer		complete HTTP container (out)
 * 		pChannel		rtsp channel for custom parameters (may be NULL)
 */
void CRtspClient::buildRequest(const char* strRequest, CHttpContainer* pContainer,
							   const CRtspChannel* pChannel)
{
	CString			strRequestUrl;
	char			strBuf[256];
	rtsp_seqnum_t	rtspSeqNum = getRtspSeq();

	if ( !m_strBaseUrl.isEmpty() )  {
		strRequestUrl = m_strBaseUrl;
		if ( pChannel )  {
			const char*		strChannelUrl;

			strChannelUrl = pChannel->getUrl();
			if ( *strChannelUrl != '\0' ) {
				strRequestUrl.appendPath(strChannelUrl);
			}
		}
	}
	else {
		shell_assert(!m_rtspServerUrl.isEmpty());
		strRequestUrl = m_rtspServerUrl;
	}

	_tsnprintf(strBuf, sizeof(strBuf), "%s %s %s", strRequest, (const char*)strRequestUrl, RTSP_VER);
	pContainer->setStartLine(strBuf);

	_tsnprintf(strBuf, sizeof(strBuf), "%s: %u", RTSP_CSEQ_HEADER, rtspSeqNum);
	pContainer->appendHeader(strBuf);

	if ( !m_rtspSession.isEmpty() )  {
		_tsnprintf(strBuf, sizeof(strBuf), "%s: %s", RTSP_SESSION_HEADER, (const char*)m_rtspSession);
		pContainer->appendHeader(strBuf);
	}

	_tsnprintf(strBuf, sizeof(strBuf), "%s: %s", RTSP_USER_AGENT_HEADER, NET_MEDIA_LIBRARY_USER_AGENT);
	pContainer->appendHeader(strBuf);
}

/*
 * Run RTSP request
 *
 * 		pContainer		http request container
 */
void CRtspClient::executeRequest(CHttpContainer* pContainer)
{
	m_sessId = getUniqueId();
	m_netClient.io(pContainer, m_sessId);
}

/*
 * Execute 'OPTIONS' request
 */
void CRtspClient::doOptions()
{
	dec_ptr<CHttpContainer>		pContainer = new CHttpContainer;

	shell_assert(!m_rtspServerUrl.isEmpty());

	log_debug(L_RTSP_FL, "[rtsp_cli] executing OPTIONS request\n");

	buildRequest("OPTIONS", pContainer, NULL);
	setFsmState(RTSP_FSM_OPTIONS);
	executeRequest(pContainer);
}

/*
 * Execute 'DESCRIBE' request
 */
void CRtspClient::doDescribe()
{
	dec_ptr<CHttpContainer>		pContainer = new CHttpContainer;
	char 						strTemp[128];

	log_debug(L_RTSP_FL, "[rtsp_cli] executing DESCRIBE request\n");

	buildRequest("DESCRIBE", pContainer, NULL);
	_tsnprintf(strTemp, sizeof(strTemp), "Accept: %s", SDP_HTTP_MEDIA_TYPE);
	pContainer->appendHeader(strTemp);

	setFsmState(RTSP_FSM_DESCRIBE);
	executeRequest(pContainer);
}

/*
 * Execute 'PLAY' request
 */
void CRtspClient::doPlayback()
{
	dec_ptr<CHttpContainer>		pContainer = new CHttpContainer;
	char	strBuf[256];

	log_debug(L_RTSP_FL, "[rtsp_cli] executing PLAY request\n");

	buildRequest("PLAY", pContainer, NULL);

	_tsnprintf(strBuf, sizeof(strBuf), "%s: %s", RTSP_RANGE_HEADER, "npt=0.000-");
	pContainer->appendHeader(strBuf);

	setFsmState(RTSP_FSM_PLAYING);
	executeRequest(pContainer);
}

/*
 * Execute 'PAUSE' request
 */
void CRtspClient::doPause()
{
	dec_ptr<CHttpContainer>		pContainer = new CHttpContainer;

	log_debug(L_RTSP_FL, "[rtsp_cli] executing PAUSE request\n");

	buildRequest("PAUSE", pContainer, NULL);
	setFsmState(RTSP_FSM_PAUSING);
	executeRequest(pContainer);
}

/*
 * Execute 'TEARDOWN' request
 */
void CRtspClient::doTeardown()
{
	dec_ptr<CHttpContainer>		pContainer = new CHttpContainer;

	log_debug(L_RTSP_FL, "[rtsp_cli] executing TEARDOWN request\n");

	buildRequest("TEARDOWN", pContainer, NULL);
	setFsmState(RTSP_FSM_TEARDOWN);
	executeRequest(pContainer);
}

/*
 * Disconnect RTSP client ftom server and close connection
 */
void CRtspClient::doDisconnect()
{
	log_debug(L_RTSP_FL, "[rtsp_cli] RTSP disconnect\n");

	m_netClient.disconnect();
	stopIdleTimer();
	setFsmState(RTSP_FSM_NONE);
	reset();
}

/*
 * Stop RTSP client 'alive' timer
 */
void CRtspClient::stopIdleTimer()
{
	SAFE_DELETE_TIMER(m_pIdleTimer, m_pOwnerLoop);
}

/*
 * Start RTSP client 'alive' timer
 */
void CRtspClient::startIdleTimer()
{
	hr_time_t	hrPeriod;

	stopIdleTimer();

	hrPeriod = SECONDS_TO_HR_TIME(m_rtspTimeout > 10 ? (m_rtspTimeout-5) : 5);

	m_pIdleTimer = new CTimer(hrPeriod, TIMER_CALLBACK(CRtspClient::onIdleTimer, this),
							  CTimer::timerPeriodic, this, "rtsp-idle");
	m_pOwnerLoop->insertTimer(m_pIdleTimer);
}

/*
 * RTSP protocol IDLE timer (hold connection)
 */
void CRtspClient::onIdleTimer(void* p)
{
	fsm_t	fsm = getFsmState();

	if ( fsm != RTSP_FSM_NONE && fsm != RTSP_FSM_CONNECTED )  {
		if ( !m_rtspSession.isEmpty() ) {
			dec_ptr<CHttpContainer>		pContainer = new CHttpContainer;

			log_debug(L_RTSP_FL, "[rtsp_cli] running IDLE timer\n");

			buildRequest((m_rtspOptions&RTSP_OPTION_GET_PARAMETER) != 0 ?
						 "GET_PARAMETER" : "OPTIONS", pContainer, NULL);
			executeRequest(pContainer);
		}
	}
}

/*
 * Process 'OPTIONS' request response
 *
 * 		pContainer		response
 *
 * Return:
 * 		ESUCCESS		request completed successful
 * 		other code		undo to previoust state
 */
result_t CRtspClient::processOptions(CHttpContainer* pContainer)
{
	static struct {
		const char*		strOpt;
		int 			value;
	} arOptions[] = {
		{ "DESCRIBE", 			RTSP_OPTION_DESCRIBE 		},
		{ "ANNOUNCE",			RTSP_OPTION_ANNOUNCE 		},
		{ "GET_PARAMETER",		RTSP_OPTION_GET_PARAMETER	},
		{ "OPTIONS",			RTSP_OPTION_OPTIONS			},
		{ "PAUSE",				RTSP_OPTION_PAUSE			},
		{ "PLAY",				RTSP_OPTION_PLAY			},
		{ "RECORD",				RTSP_OPTION_RECORD			},
		{ "REDIRECT",			RTSP_OPTION_REDIRECT		},
		{ "SETUP",				RTSP_OPTION_SETUP			},
		{ "SET_PARAMETER",		RTSP_OPTION_SET_PARAMETER	},
		{ "TEARDOWN",			RTSP_OPTION_TEARDOWN		}
	};

	CString		strResponse, strHeader, strOpt;
	const char	*s, *p;
	size_t		i, count;
	result_t	nresult;

	shell_assert(m_rtspOptions == 0);
	m_rtspOptions = 0;

	strResponse = pContainer->getStart();
	if ( strResponse != RTSP_SUCCESS_RESPONSE )  {
		log_debug(L_RTSP, "[rtsp_cli] invalid OPTIONS response code '%s'\n", strResponse.cs());
		return EINVAL;
	}

	if ( logger_is_enabled(LT_DEBUG|L_RTSP_FL) )  {
		CString		strHead;

		pContainer->getHeader(strHead);
		log_debug(L_RTSP_FL, "[rtsp_cli] request OPTIONS successful, response:\n%s", strHead.cs());
	}

	if ( pContainer->getHeader(strHeader, RTSP_SERVER_HEADER) )  {
		log_debug(L_RTSP, "[rtsp_cli] detected RTSP server: %s\n", strHeader.cs());
	}

	pContainer->getHeader(strHeader, "Public");
	s = strHeader;

	while ( *s != '\0' )  {
		p = _tstrchr(s, ',');
		if ( p )  {
			strOpt = CString(s, A(p)-A(s));
			s = p+1;
		}
		else {
			strOpt = s;
			s += _tstrlen(s);
		}

		strOpt.trim();
		count = ARRAY_SIZE(arOptions);
		for(i=0; i<count; i++)  {
			if ( strOpt.equalNoCase(arOptions[i].strOpt) )  {
				m_rtspOptions |= arOptions[i].value;
			}
		}
	}

	if ( (m_rtspOptions&RTSP_REQUIRED_OPTIONS) == RTSP_REQUIRED_OPTIONS ) {
		log_debug(L_RTSP_FL, "[rtsp_cli] RTSP server supported options %Xh\n", m_rtspOptions);
		doDescribe();
		nresult = ESUCCESS;
	}
	else {
		log_error(L_RTSP, "[rtsp_cli] missing RTSP protocol required options (%Xh, required %Xh)\n",
				  m_rtspOptions, RTSP_REQUIRED_OPTIONS);
		nresult = EINVAL;
	}

	return nresult;
}

/*
 * Process 'DESCRIBE' request response
 *
 * 		pContainer		response
 *
 * Return:
 * 		ESUCCESS		request completed successful
 * 		other code		undo to previoust state
 */
result_t CRtspClient::processDescribe(CHttpContainer* pContainer)
{
	CString		strResponse, strHead;
	const char*	pBody;
	result_t	nresult;

	shell_assert(m_strBaseUrl.isEmpty());
	shell_assert(m_rtspOptions != 0);

	/* Check response code */
	strResponse = pContainer->getStart();
	if ( strResponse != RTSP_SUCCESS_RESPONSE )  {
		log_debug(L_RTSP, "[rtsp_cli] invalid DESCRIBE response '%s'\n", strResponse.cs());
		return EINVAL;
	}

	/* Check Content-Type header (optional) */
	if ( pContainer->getHeader(strHead, RTSP_CONTENT_TYPE_HEADER) ) {
		if ( strHead != SDP_HTTP_MEDIA_TYPE ) {
			log_error(L_RTSP, "[rtsp_cli] invalid DESCRIBE content-type: '%s', expected '%s'\n",
						strHead.cs(), SDP_HTTP_MEDIA_TYPE);
			return EINVAL;
		}
	}

	pBody = (const char*)pContainer->getBody();
	if ( !pBody ) {
		log_error(L_RTSP, "[rtsp_cli] missing body of the DESCRIBE request\n");
		return EINVAL;
	}

	log_debug(L_RTSP_FL, "[rtsp_cli] request DESCRIBE successful, content:\n%s", pBody);

	m_sdp.clear();
	nresult = m_sdp.load(pBody);
	if ( nresult == ESUCCESS )  {
		/* Determine Base-Content URL */
		if ( pContainer->getHeader(strHead, RTSP_CONTENT_BASE_HEADER) )  {
			m_strBaseUrl = strHead;
		} else
		if ( pContainer->getHeader(strHead, RTSP_CONTENT_LOCATION_HEADER) ) {
			m_strBaseUrl = strHead;
		} else
		if ( pContainer->getHeader(strHead, RTSP_REQUEST_URI_HEADER) )  {
			m_strBaseUrl = strHead;
		}

		if ( !m_strBaseUrl.isEmpty() )  {
			log_debug(L_RTSP, "[rtsp_cli] detected RTSP base URL: %s\n", m_strBaseUrl.cs());
		}

		/*
		 * Connection sequence completed
		 */
		setFsmState(RTSP_FSM_CONNECTED);
		m_pParent->onRtspConnect(this, ESUCCESS);
	}
	else {
		log_error(L_RTSP, "[rtsp_cli] failed to parse SDP, result %d\n", nresult);
	}

	return nresult;
}

/*
 * Process 'SETUP' request response
 *
 * 		pContainer		response
 *
 * Return:
 * 		ESUCCESS		request completed successful
 * 		other code		undo to previoust state
 */
result_t CRtspClient::processSetup(CHttpContainer* pContainer)
{
	CString			strResponse, strHead, strSession;
	int 			timeout;
	result_t		nresult;

	shell_assert(m_rtspSession.isEmpty());
	shell_assert(m_pAsyncChannel);

	/* Check response code */
	strResponse = pContainer->getStart();
	if ( strResponse != RTSP_SUCCESS_RESPONSE )  {
		log_debug(L_RTSP, "[rtsp_cli] invalid SETUP response '%s'\n", strResponse.cs());
		return EINVAL;
	}

	pContainer->getHeader(strHead);

	log_debug(L_RTSP_FL, "[rtsp_cli] request SETUP successful, response:\n%s", strHead.cs());

	/*
	 * Parse Session/Timeout header
	 */
	if ( pContainer->getHeader(strHead, RTSP_SESSION_HEADER) ) {
		parseSession(strHead, strSession, timeout);
		if ( !strSession.isEmpty() )  {
			m_rtspSession = strSession;
			log_debug(L_RTSP_FL, "[rtsp_cli] RTSP session: '%s'\n", m_rtspSession.cs());
		}
		if ( timeout > 0 )  {
			m_rtspTimeout = timeout;
			log_debug(L_RTSP, "[rtsp_cli] RTSP session alive interval %d seconds\n", m_rtspTimeout);
		}
	}

	if ( m_rtspSession.isEmpty() )  {
		log_error(L_RTSP, "[rtsp_cli] no RTSP session specified\n");
		return EINVAL;
	}

	log_debug(L_RTSP_FL, "[rtsp_cli] RTSP timeout: %d secs\n", m_rtspTimeout);

	/*
	 * Parse and check server selected transport parameters:
	 * 		Transport: <transport>;<unicast>;<client_port=nnnn-nnnn>;<server_port=nnnn-nnnn>;<ssrc=nnnnnnnn>
	 */
	if ( !pContainer->getHeader(strHead, RTSP_TRANSPORT_HEADER) )  {
		log_error(L_RTSP, "[rtsp_cli] no transport parameters in the server response\n");
		return EINVAL;
	}

	nresult = m_pAsyncChannel->checkSetupResponse(strHead, &m_sdp);
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	setFsmState(RTSP_FSM_CONFIGURED);
	startIdleTimer();
	m_pParent->onRtspConfigure(this, ESUCCESS);
	return ESUCCESS;
}

/*
 * Process 'PLAY' request response
 *
 * 		pContainer		response
 *
 * Return:
 * 		ESUCCESS		request completed successful
 * 		other code		undo to previoust state
 */
result_t CRtspClient::processPlay(CHttpContainer* pContainer)
{
	CString			strResponse;

	/* Check response code */
	strResponse = pContainer->getStart();
	if ( strResponse != RTSP_SUCCESS_RESPONSE )  {
		log_debug(L_RTSP, "[rtsp_cli] invalid PLAY response '%s'\n", strResponse.cs());
		return EINVAL;
	}

	if ( logger_is_enabled(LT_DEBUG|L_RTSP_FL) )  {
		CString		strHead;

		pContainer->getHeader(strHead);
		log_debug(L_RTSP_FL, "[rtsp_cli] request PLAY successful, response:\n%s", strHead.cs());
	}

	setFsmState(RTSP_FSM_PLAYBACK);
	m_pParent->onRtspPlay(this, ESUCCESS);
	return ESUCCESS;
}

/*
 * Process 'PAUSE' request response
 *
 * 		pContainer		response
 *
 * Return:
 * 		ESUCCESS		request completed successful
 * 		other code		undo to previoust state
 */
result_t CRtspClient::processPause(CHttpContainer* pContainer)
{
	CString			strResponse;

	/* Check response code */
	strResponse = pContainer->getStart();
	if ( strResponse != RTSP_SUCCESS_RESPONSE )  {
		log_debug(L_RTSP, "[rtsp_cli] invalid PAUSE response '%s'\n", strResponse.cs());
		return EINVAL;
	}

	log_debug(L_RTSP_FL, "[rtsp_cli] request PAUSE successful\n");

	setFsmState(RTSP_FSM_CONFIGURED);
	m_pParent->onRtspPause(this, ESUCCESS);
	return ESUCCESS;
}

/*
 * Process 'TEARDOWN' request response
 *
 * 		pContainer		response
 *
 * Return:
 * 		ESUCCESS		request completed successful
 * 		other code		undo to previoust state
 */
result_t CRtspClient::processTeardown(CHttpContainer* pContainer)
{
	CString			strResponse;

	/* Check response code */
	strResponse = pContainer->getStart();
	if ( strResponse == RTSP_SUCCESS_RESPONSE ) {
		log_debug(L_RTSP_FL, "[rtsp_cli] request TEARDOWN successful, response\n%s\n",
				  strResponse.cs());
	}
	else {
		log_warning(L_RTSP, "[rtsp_cli] invalid TEARDOWN response:\n%s\n",
					strResponse.cs());
	}

	doDisconnect();
	m_pParent->onRtspDisconnect(this, ESUCCESS);
	return ESUCCESS;
}

/*******************************************************************************/

/*
 * Connect to the RTSP server
 *
 * 		rtspServerAddr			RTSP server address
 * 		strServerUrl			RTSP server URL
 */
void CRtspClient::connect(const CNetAddr& rtspServerAddr, const char* strServerUrl)
{
	fsm_t		fsm = getFsmState();

	if ( fsm != RTSP_FSM_NONE )  {
		log_debug(L_RTSP, "[rtsp_cli] can't connect, client BUSY, state %d\n", fsm);
		m_pParent->onRtspConnect(this, EBUSY);
		return;
	}

	shell_assert(!m_rtspServerAddr.isValid());
	shell_assert(m_rtspServerUrl.isEmpty());
	shell_assert(m_strBaseUrl.isEmpty());
	shell_assert(m_rtspOptions == 0);
	shell_assert(m_rtspSession.isEmpty());
	shell_assert(m_rtspSeq == 0);

	m_rtspServerAddr = rtspServerAddr;
	m_rtspServerUrl = strServerUrl;

	log_debug(L_RTSP_FL, "[rtsp_cli] connecting to %s\n", (const char*)m_rtspServerAddr);

	if ( !m_rtspServerAddr.isValid() )  {
		log_error(L_RTSP, "[rtsp_cli] invalid RTSP address\n");
		m_pParent->onRtspConnect(this, EINVAL);
		return;
	}

	setFsmState(RTSP_FSM_CONNECT);
	m_sessId = getUniqueId();

	/* Set optional bind address */
	if ( m_selfHost.isValid() )  {
		CNetAddr	netAddr(m_selfHost, (ip_port_t)0);
		m_netClient.setBindAddr(netAddr);
	}

	m_netClient.connect(m_rtspServerAddr, m_sessId);
}

/*
 * Configure a media channel
 *
 * 		pChannel		RTSP media channel to configure
 */
void CRtspClient::configure(CRtspChannel* pChannel)
{
	dec_ptr<CHttpContainer>	pContainer = new CHttpContainer;
	fsm_t					fsm = getFsmState();
	char					strTemp[256];

	shell_assert(pChannel->isEnabled());
	shell_assert(pChannel->getMediaIndex() != RTSP_MEDIA_INDEX_UNDEF);

	shell_assert(m_rtspSession.isEmpty());

	if ( fsm != RTSP_FSM_CONNECTED && fsm != RTSP_FSM_CONFIGURED )  {
		log_debug(L_RTSP, "[rtsp_cli] can't configure, client BUSY, state %d\n", fsm);
		m_pParent->onRtspConfigure(this, EBUSY);
		return;
	}

	log_debug(L_RTSP_FL, "[rtsp_cli] executing SETUP request\n");

	buildRequest("SETUP", pContainer, pChannel);
	pChannel->getSetupRequest(strTemp, sizeof(strTemp), &m_sdp);
	pContainer->appendHeader(strTemp);

	setFsmState(RTSP_FSM_SETUP);
	m_pAsyncChannel = pChannel;
	executeRequest(pContainer);
}

/*
 * Start streaming all channels in the session
 */
void CRtspClient::play()
{
	fsm_t	fsm = getFsmState();

	switch ( fsm )  {
		case RTSP_FSM_PLAYBACK:
			m_pParent->onRtspPlay(this, ESUCCESS);
			break;

		case RTSP_FSM_PLAYING:
			break;

		case RTSP_FSM_CONFIGURED:
		case RTSP_FSM_PAUSING:
			doPlayback();
			break;

		default:
			log_debug(L_RTSP, "[rtsp_cli] can't play, client BUSY, state %d\n", fsm);
			m_pParent->onRtspPlay(this, EBUSY);
			break;
	}
}

/*
 * Stop streaming all channels in the session
 */
void CRtspClient::pause()
{
	fsm_t	fsm = getFsmState();

	switch ( fsm )  {
		case RTSP_FSM_CONFIGURED:
			m_pParent->onRtspPause(this, ESUCCESS);
			break;

		case RTSP_FSM_PAUSING:
			break;

		case RTSP_FSM_PLAYBACK:
		case RTSP_FSM_PLAYING:
			if ( m_rtspOptions&RTSP_OPTION_PAUSE ) {
				doPause();
			}
			else {
				log_debug(L_RTSP, "[rtsp_cli] PAUSE request is not supported by RTSP server\n");
				m_pParent->onRtspPause(this, ENOSYS);
			}
			break;

		default:
			log_debug(L_RTSP, "[rtsp_cli] can't pause, client BUSY, state %d\n", fsm);
			m_pParent->onRtspPause(this, EBUSY);
			break;
	}
}

/*
 * Close RTSP session and disconnect from server
 */
void CRtspClient::disconnect()
{
	fsm_t	fsm = getFsmState();

	switch ( fsm )  {
		case RTSP_FSM_NONE:
			m_pParent->onRtspDisconnect(this, ESUCCESS);
			break;

		case RTSP_FSM_CONNECTED:
		case RTSP_FSM_CONNECT:
		case RTSP_FSM_OPTIONS:
		case RTSP_FSM_DESCRIBE:
		case RTSP_FSM_SETUP:
		default:
			doDisconnect();
			m_pParent->onRtspDisconnect(this, ESUCCESS);
			break;

		case RTSP_FSM_CONFIGURED:
		case RTSP_FSM_PLAYBACK:
		case RTSP_FSM_PLAYING:
		case RTSP_FSM_PAUSING:
			doTeardown();
			break;

		case RTSP_FSM_TEARDOWN:
			break;
	}
}

/*
 * Initialise RTSP protocol client
 *
 * Return: ESUCCESS, ...
 */
result_t CRtspClient::init()
{
	result_t	nresult;

	nresult = CModule::init();
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

 	nresult = m_netClient.init();
	if ( nresult != ESUCCESS )  {
		CModule::terminate();
	}

	reset();

	return nresult;
}

/*
 * Terminate RTSP protocol client
 */
void CRtspClient::terminate()
{
	m_netClient.terminate();
	CModule::terminate();
}

/*******************************************************************************
 * Debugging support
 */

void CRtspClient::dump(const char* strPref) const
{
	fsm_t	fsm = getFsmState();

	log_dump("*** %sRTSP client: fsm: %d, selfHost: %s, RTSP server: (%s) '%s'\n",
			 strPref, fsm, (const char*)m_selfHost, (const char*)m_rtspServerAddr,
			 m_rtspServerUrl.cs());
	log_dump("    options: %02Xh, session: '%s', timeout: %d secs\n",
	         m_rtspOptions, m_rtspSession.cs(), m_rtspTimeout);
	log_dump("    base url: '%s', idle timer: %s\n",
			 m_strBaseUrl.cs(), m_pIdleTimer ? "ON" : "OFF");
	m_sdp.dump();
}
