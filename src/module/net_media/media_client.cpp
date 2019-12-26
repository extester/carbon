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

#include "net_media/media_client.h"

enum {
	CLIENT_FSM_NONE,			/* Uninitialised client */
	CLIENT_FSM_CONNECTED,		/* Connected to RTSP server, performed OPTIONS/DESCRIBE */
	CLIENT_FSM_CONFIGURED,		/* Configured/Paused, ready for PLAY/PAUSE/TEARDOWN */
	CLIENT_FSM_PLAYBACK,		/* Playing */

	CLIENT_FSM_CONNECTING,		/* Connecting to RTSP server */
	CLIENT_FSM_CONFIGURING,		/* Setting up media channel parameters */
	CLIENT_FSM_PLAYING,			/* Performing PLAY request */
	CLIENT_FSM_PAUSING,			/* Performing PAUSE request */
	CLIENT_FSM_DISCONNECTING	/* Disconnecting */
};

/*******************************************************************************
 * CMediaClient class
 */

CMediaClient::CMediaClient(const CNetHost& selfHost, CEventLoop* pOwnerLoop,
						   IMediaClient* pParent, const char* strName) :
	CModule(strName),
	CStateMachine(CLIENT_FSM_NONE),
	IRtspClient(),
	m_pOwnerLoop(pOwnerLoop),
	m_pParent(pParent),
	m_selfHost(selfHost),
	m_rtsp(selfHost, pOwnerLoop, this),
	m_rtcp(selfHost, pOwnerLoop, &m_sessionMan),
	m_asyncChannelIndex(-1),
	m_asyncResult(EINVAL),
	m_pReceiverPool(new CRtpReceiverPool(strName))
{
}

CMediaClient::~CMediaClient()
{
	fsm_t	fsm = getFsmState();

	shell_assert_ex(fsm == CLIENT_FSM_NONE, "fsm: %d ", fsm); UNUSED(fsm);

	shell_assert_ex(m_arChannel.empty(), "[media_cli(%s)] where are %d channel(s) exist",
						getName(), m_arChannel.size());

	SAFE_DELETE(m_pReceiverPool);
}

/*
 * Reset client state
 */
void CMediaClient::reset()
{
	m_pReceiverPool->reset();
	m_serverAddr = CNetHost();

	m_asyncChannelIndex = -1;
	m_asyncResult = EINVAL;
}

/*
 * Insert media channel to the RTP stream
 *
 * 		pChannel		media channel to insert
 *
 * Return: ESUCCESS, EINVAL
 */
result_t CMediaClient::insertChannel(CRtspChannel* pChannel)
{
	fsm_t	fsm = getFsmState();

	shell_assert(pChannel);

	if ( fsm != CLIENT_FSM_NONE )  {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] can't insert channel after init()\n", getName());
		return EINVAL;
	}

	pChannel->setSessionManager(&m_sessionMan);
	m_arChannel.push_back(pChannel);

	return ESUCCESS;
}

/*
 * Remove specified media channel from the RTP stream
 *
 * 		pChannel		media channel descriptor to remove
 */
void CMediaClient::removeChannel(CRtspChannel* pChannel)
{
	fsm_t	fsm = getFsmState();
	size_t	i, count;

	shell_assert(pChannel);

	if ( fsm != CLIENT_FSM_NONE )  {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] can't delete channel id:%u, client BUSY\n",
				  	getName(), pChannel->getId());
		return;
	}

	count = m_arChannel.size();
	for(i=0; i<count; i++)  {
		if ( m_arChannel[i] == pChannel )  {
			m_arChannel.erase(m_arChannel.begin()+i);
			break;
		}
	}
}

/*
 * Disable all enabled channels
 */
void CMediaClient::disableRtpChannels()
{
	size_t		i, count = m_arChannel.size();

	for(i=0; i<count; i++) {
		if ( m_arChannel[i]->isEnabled()) {
			m_arChannel[i]->disableRtp();
		}
	}
}

/*
 * Initialise and start RTP protocol
 *
 * Return: ESUCCESS, ...
 */
result_t CMediaClient::enableRtp()
{
	size_t		i, count = m_arChannel.size();
	result_t	nresult;

	m_pReceiverPool->terminate();

	for(i=0; i<count;i++)  {
		if ( m_arChannel[i]->isEnabled() )  {
			CNetAddr	netAddr(m_selfHost, m_arChannel[i]->getRtpPort());

			m_pReceiverPool->insertChannel(m_arChannel[i]->getPlayoutBuffer(), netAddr);
		}
	}

	nresult = m_pReceiverPool->init();
	if ( nresult != ESUCCESS )  {
		m_pReceiverPool->removeAllChannels();

		log_error(L_NET_MEDIA, "[media_cli(%s)] failed to init RTP receiver, result %d\n",
				  	getName(), nresult);
		return nresult;
	}

	return ESUCCESS;
}

/*
 * Stop RTP protocol
 */
void CMediaClient::disableRtp()
{
	m_pReceiverPool->terminate();
	m_pReceiverPool->removeAllChannels();
	disableRtpChannels();
}

uint32_t CMediaClient::getServerSsrc() const
{
	uint32_t	ssrc = 0;
	size_t		i, count = m_arChannel.size();

	for(i=0; i<count;i++)  {
		if ( m_arChannel[i]->isEnabled() )  {
			ssrc = m_arChannel[i]->getServerSsrc();
			if ( ssrc ) {
				break;
			}
		}
	}

	return ssrc;
}


/*
 * Initialise and start RTCP protocol
 *
 * Return: ESUCCESS, ...
 */
result_t CMediaClient::enableRtcp()
{
	ip_port_t			nRtpPort = 0, nRtpServerPort = 0;
	size_t				i, count = m_arChannel.size();
	uint32_t			ssrc;
	result_t			nresult;

	for(i=0; i<count;i++)  {
		if ( m_arChannel[i]->isEnabled() )  {
			nRtpPort = m_arChannel[i]->getRtpPort();
			nRtpServerPort = m_arChannel[i]->getRtpServerPort();
			break;
		}
	}

	if ( nRtpPort == 0 )  {
		log_error(L_NET_MEDIA, "[media_cli(%s)] NO RTCP PORT\n", getName());
		return EINVAL;
	}

	ssrc = getServerSsrc();
	if ( !ssrc )  {
		log_error(L_NET_MEDIA, "[media_cli(%s)] no server SSRC is defined\n", getName());
		return EINVAL;
	}

	nresult = m_rtcp.init(m_serverAddr, nRtpPort+1, nRtpServerPort+1, ssrc);
	if ( nresult != ESUCCESS )  {
		log_error(L_NET_MEDIA, "[media_cli(%s)] failed to init RTCP, result %d\n",
				  	getName(), nresult);
		return nresult;
	}

	return ESUCCESS;
}

void CMediaClient::disableRtcp()
{
	m_rtcp.terminate();
}


/*
 * Connect to media server and obtain the supported media descriptions
 *
 * 		rtspServerAddr			RTSP server address
 * 		strServerUrl			RTSP server URL
 *
 * Result: call m_pParent->onConnect(this, nresult);
 */
void CMediaClient::connect(const CNetAddr& rtspServerAddr, const char* strServerUrl)
{
	fsm_t	fsm = getFsmState();

	if ( fsm != CLIENT_FSM_NONE )  {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] can't connect, client BUSY, state %d\n", getName(), fsm);
		m_pParent->onMediaClientConnect(this, EBUSY);
		return;
	}

	log_trace(L_NET_MEDIA, "[media_cli(%s)] connecting to RTSP server %s (%s)\n",
			  	getName(), rtspServerAddr.c_str(), strServerUrl);

	m_serverAddr = CNetHost((ip_addr_t)rtspServerAddr);
	setFsmState(CLIENT_FSM_CONNECTING);
	m_rtsp.connect(rtspServerAddr, strServerUrl);
}

/*
 * Configure next enabled media channel
 */
void CMediaClient::configureNext()
{
	result_t	nr, nresult;
	size_t		i, count = m_arChannel.size();

	shell_assert(getFsmState() == CLIENT_FSM_CONFIGURING);

	m_asyncChannelIndex++;
	if ( m_asyncChannelIndex < (int)count )  {
		boolean_t	bFound = FALSE;

		for(i=(size_t)m_asyncChannelIndex; i<count; i++)  {
			CRtspChannel*	pd = m_arChannel[i];
			int 				nMediaIndex;

			if ( pd->isEnabled() )  {
				log_trace(L_NET_MEDIA, "[media_cli(%s)] configuring channel #%u (id=%u)...\n",
						  	getName(), i, pd->getId());

				pd->reset();
				nMediaIndex = m_rtsp.initChannel(pd);
				if ( nMediaIndex != RTSP_MEDIA_INDEX_UNDEF ) {
					log_trace(L_NET_MEDIA, "[media_cli(%s)] channel #%u (id=%u): media found (index %d)\n",
							  	getName(), i, pd->getId(), nMediaIndex);

					nr = pd->enableRtp();
					if ( nr == ESUCCESS ) {
						m_asyncChannelIndex = (int)i;
						m_rtsp.configure(pd);
						bFound = TRUE;
						break;
					}
					else {
						log_debug(L_NET_MEDIA, "[media_cli(%s)] can't init RTP for channel #%u (id=%u), "
								"result: %d, DISABLED\n", getName(), i, pd->getId(), nr);
						pd->reset();
						pd->enable(FALSE);
					}
				}
				else {
					log_debug(L_NET_MEDIA, "[media_cli(%s)] no compatible media found, channel #%u (id=%u), DISABLED\n",
							  	getName(), i, pd->getId());
					pd->enable(FALSE);
				}
			}
		}

		if ( bFound )  {
			return;
		}
	}

	/*
	 * No more channels to configure
	 */
	nresult = m_asyncResult;
	if ( nresult == ESUCCESS )  {
		int 	enabledCount = 0;
		size_t	j;

		for(j=0; j<count; j++)  {
			enabledCount += m_arChannel[j]->isEnabled();
		}

		if ( enabledCount == 0 )  {
			/* No enabled channels found */
			nresult = EFAULT;
			log_warning(L_NET_MEDIA, "[media_cli(%s)] ALL channels are disabled\n", getName());
		}
	}

	if ( nresult == ESUCCESS )  {
		/*
		 * All channels were configured and at least one channel is enabled
		 */
		nresult = enableRtp();
		if ( nresult == ESUCCESS )  {
			nresult = enableRtcp();
			if ( nresult != ESUCCESS )  {
				disableRtp();
				disableRtpChannels();
			}
		}
		else {
			disableRtpChannels();
		}
	}

	setFsmState(nresult == ESUCCESS ? CLIENT_FSM_CONFIGURED : CLIENT_FSM_CONNECTED);
	m_pParent->onMediaClientConfigure(this, nresult);
}

/*
 * Configure media descriptors
 */
void CMediaClient::configure()
{
	fsm_t	fsm = getFsmState();

	if ( fsm != CLIENT_FSM_CONNECTED )  {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] can't configure, client BUSY, state %d\n",
				  	getName(), fsm);
		m_pParent->onMediaClientConfigure(this, EBUSY);
		return;
	}

	setFsmState(CLIENT_FSM_CONFIGURING);
	m_asyncChannelIndex = -1;
	m_asyncResult = ESUCCESS;
	configureNext();
}

/*
 * Start streaming all configured channels in the session
 */
void CMediaClient::play()
{
	fsm_t	fsm = getFsmState();

	switch ( fsm )  {
		case CLIENT_FSM_PLAYBACK:
			m_pParent->onMediaClientPlay(this, ESUCCESS);
			break;

		case CLIENT_FSM_PLAYING:
			/* PLAY request is in progress */
			break;

		case CLIENT_FSM_CONFIGURED:
		case CLIENT_FSM_PAUSING:
			setFsmState(CLIENT_FSM_PLAYING);
			m_rtsp.play();
			break;

		default:
			log_error(L_NET_MEDIA, "[media_cli(%s)] can't play, net media BUSY, state %d\n",
					  	getName(), fsm);
			m_pParent->onMediaClientPlay(this, EBUSY);
	}
}

/*
 * Stop streaming all configured channels in the session
 */
void CMediaClient::pause()
{
	fsm_t	fsm = getFsmState();

	switch ( fsm )  {
		case CLIENT_FSM_CONFIGURED:
			m_pParent->onMediaClientPause(this, ESUCCESS);
			break;

		case CLIENT_FSM_PAUSING:
			/* PAUSE request is in progress */
			break;

		case CLIENT_FSM_PLAYBACK:
		case CLIENT_FSM_PLAYING:
			setFsmState(CLIENT_FSM_PAUSING);
			m_rtsp.pause();
			break;

		default:
			log_error(L_NET_MEDIA, "[media_cli(%s)] can't play, client BUSY, state %d\n",
					  	getName(), fsm);
			m_pParent->onMediaClientPlay(this, EBUSY);
			break;
	}
}

/*
 * Close session and disconnect from the RTSP server
 */
void CMediaClient::disconnect()
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CLIENT_FSM_NONE )  {
		m_pParent->onMediaClientDisconnect(this, ESUCCESS);
	} else if ( fsm == CLIENT_FSM_DISCONNECTING )  {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] already disconnecting\n", getName());
	} else {
		log_trace(L_NET_MEDIA, "[media_cli(%s)] disconnecting\n", getName());
		disableRtcp();
		disableRtp();
		setFsmState(CLIENT_FSM_DISCONNECTING);
		m_rtsp.disconnect();
	}
}


/*******************************************************************************
 * RTSP result handlers
 */

void CMediaClient::onRtspConnect(CRtspClient* pRtspClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CLIENT_FSM_CONNECTING ) {
		log_trace(L_NET_MEDIA, "[media_cli(%s)] connected RTSP server, result: %d\n",
				  	getName(), nresult);

		setFsmState(nresult == ESUCCESS ? CLIENT_FSM_CONNECTED : CLIENT_FSM_NONE);
		m_pParent->onMediaClientConnect(this, nresult);
	}
	else {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] fsm incorrect (%d/%d)\n", getName(), fsm, nresult);
	}
}

void CMediaClient::onRtspConfigure(CRtspClient* pRtspClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CLIENT_FSM_CONFIGURING )  {
		CRtspChannel*		pd = 0;
		uint32_t			id = 0xffffffff;

		if ( m_asyncChannelIndex < (int)m_arChannel.size() )  {
			pd = m_arChannel[m_asyncChannelIndex];
			id = pd->getId();
		}

		if ( nresult == ESUCCESS ) {
			log_trace(L_NET_MEDIA, "[media_cli(%s)] configured client %u (id=%u)\n",
					  	getName(), m_asyncChannelIndex, id);
		}
		else {
			log_error(L_NET_MEDIA, "[media_cli(%s)] client %u (id=%u) configuration failed, "
				      "result %d, DISABLE\n", getName(), m_asyncChannelIndex, id, nresult);
			if ( pd )  {
				pd->disableRtp();
				pd->reset();
				pd->enable(FALSE);
			}
		}
		nresult_join(m_asyncResult, nresult);

		configureNext();
	}
	else {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] fsm incorrect (%d/%d)\n", getName(), fsm, nresult);
	}
}

void CMediaClient::onRtspPlay(CRtspClient* pRtspClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CLIENT_FSM_PLAYING )  {
		log_trace(L_NET_MEDIA, "[media_cli(%s)] playback result: %d\n", getName(), nresult);

		setFsmState(nresult == ESUCCESS ? CLIENT_FSM_PLAYBACK : CLIENT_FSM_CONFIGURED);
		m_pParent->onMediaClientPlay(this, nresult);
	}
	else {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] fsm incorrect (%d/%d)\n", getName(), fsm, nresult);
	}
}

void CMediaClient::onRtspPause(CRtspClient* pRtspClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CLIENT_FSM_PAUSING )  {
		log_trace(L_NET_MEDIA, "[media_cli(%s)] pause result: %d\n", getName(), nresult);

		setFsmState(CLIENT_FSM_CONFIGURED);
		m_pParent->onMediaClientPause(this, nresult);
	}
	else {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] fsm incorrect (%d/%d)\n", getName(), fsm, nresult);
	}
}

void CMediaClient::onRtspDisconnect(CRtspClient* pRtspClient, result_t nresult)
{
	fsm_t	fsm = getFsmState();

	if ( fsm == CLIENT_FSM_DISCONNECTING )  {
		log_trace(L_NET_MEDIA, "[media_cli(%s)] disconnected, result: %d\n", getName(), nresult);

		reset();
		setFsmState(CLIENT_FSM_NONE);
		m_pParent->onMediaClientDisconnect(this, nresult);
	}
	else {
		log_debug(L_NET_MEDIA, "[media_cli(%s)] fsm incorrect (%d/%d)\n", getName(), fsm, nresult);
	}
}

/*
 * Initialise NetMedia client
 *
 * 	Return: ESUCCESS, ...
 */
result_t CMediaClient::init()
{
	result_t	nresult;

	nresult = CModule::init();
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	shell_assert(!m_pReceiverPool->isReceiving());

	log_trace(L_NET_MEDIA, "[media_cli(%s)] initialising (host %s)\n",
			  	getName(), m_selfHost.c_str());

	reset();

	if ( !m_selfHost.isValid() )  {
		log_error(L_NET_MEDIA, "[media_cli(%s)] invalid self network address specified\n", getName());
		return EINVAL;
	}

	nresult = m_rtsp.init();
	if ( nresult != ESUCCESS )  {
		log_error(L_NET_MEDIA, "[media_cli(%s)] RTSP protocol client initialisation failure, "
			        "result %d\n", getName(), nresult);
	}

	return nresult;
}

/*
 * Terminate NetMedia client
 */
void CMediaClient::terminate()
{
	fsm_t	fsm = getFsmState();

	log_trace(L_NET_MEDIA, "[media_cli(%s)] terminating (host %s)\n", getName(), m_selfHost.c_str());

	shell_assert_ex(fsm == CLIENT_FSM_NONE, "state %d\n", fsm); UNUSED(fsm);

	shell_assert(!m_pReceiverPool->isReceiving());
	m_rtsp.terminate();
	CModule::terminate();
}

/*******************************************************************************
 * Debugging support
 */

void CMediaClient::dump(const char* strPref) const
{
	size_t	i, count = m_arChannel.size();
	fsm_t	fsm = getFsmState();

	log_dump("==========\n");
	log_dump("*** %sNetMediaClient: name %s: %d channel(s), self: %s, rtsp server: %s, fsm: %d\n",
			 strPref, getName(), count, m_selfHost.c_str(), m_serverAddr.c_str(), fsm);
	log_dump("    asyncIndex: %d, asyncResult: %d\n",
			 m_asyncChannelIndex, m_asyncResult);

	m_pReceiverPool->dump();
	log_dump("-----------------------------------------------------\n");

	m_rtsp.dump();
	log_dump("-----------------------------------------------------\n");

	m_rtcp.dump();
	log_dump("-----------------------------------------------------\n");

	m_sessionMan.dump();
	log_dump("-----------------------------------------------------\n");

	log_dump("*** NetMedia channels(%d):\n", count);
	for(i=0; i<count; i++) {
		m_arChannel[i]->dump();
	}
	log_dump("==========\n");
}
