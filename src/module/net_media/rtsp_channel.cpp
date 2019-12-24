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
 *	Revision 1.0, 15.10.2016 17:02:58
 *		Initial revision.
 */

#include "carbon/utils.h"

#include "net_media/rtp_playout_buffer.h"
#include "net_media/rtsp_channel.h"

/*******************************************************************************
 * CRtspChannel class
 */

CRtspChannel::CRtspChannel(uint32_t id, ip_port_t nRtpPort,
						   CRtpPlayoutBuffer* pPlayoutBuffer, CVideoSink* pSink,
						   const char* strName) :
	m_id(id),
	m_strName(strName),
	m_nRtpPort(nRtpPort),
	m_bEnabled(TRUE),

	m_nMediaIndex(-1),
	m_nActualRtpPort(0),
	m_nServerRtpPort(0),

	m_nServerSsrc(0),

	m_pPlayoutBuffer(pPlayoutBuffer),
	m_pSink(pSink)
{
	shell_assert((nRtpPort&1) == 0);		/* Even port number */
}

CRtspChannel::~CRtspChannel()
{
	SAFE_DELETE(m_pPlayoutBuffer);
}

void CRtspChannel::setSessionManager(CRtpSessionManager* pSessionMan) {
	m_pPlayoutBuffer->setSessionManager(pSessionMan);
}

/*
 * Reset channel state to uninitialised
 */
void CRtspChannel::reset()
{
	m_bEnabled = TRUE;
	m_nMediaIndex = RTSP_MEDIA_INDEX_UNDEF;
	m_nActualRtpPort = 0;
	m_nServerRtpPort = 0;

	m_nServerSsrc = 0;
}

/*
 * Enable RTP/RTCP subsystem to receive RTP frames
 *
 * Return: ESUCCESS, ...
 */
result_t CRtspChannel::enableRtp()
{
	result_t	nresult;

	nresult = m_pSink->init(m_pPlayoutBuffer->getFps(), m_pPlayoutBuffer->getClockRate());
	if ( nresult == ESUCCESS )  {
		nresult = m_pPlayoutBuffer->init(m_pSink);
		if ( nresult != ESUCCESS )  {
			m_pSink->terminate();
		}
	}

	return nresult;
}

/*
 * Disable RTP/RTCP subsystem
 */
void CRtspChannel::disableRtp()
{
	m_pPlayoutBuffer->terminate();
	m_pSink->terminate();
}

result_t CRtspChannel::checkSetupResponse(const char* strResponse, const CSdp* pSdp)
{
	CString			strSsrc;
	unsigned int	ssrc;
	boolean_t		bresult;

	bresult = strParseSemicolonKeyValue(strResponse, "ssrc", strSsrc, " ");
	if ( !bresult )  {
		log_debug(L_RTSP, "[rtsp_ch(%s)] media %d: server ssrc is not found\n",
				  m_strName.cs(), m_nMediaIndex);
		return EINVAL;
	}

	if ( strSsrc.getNumber(ssrc, 16) != ESUCCESS )  {
		log_debug(L_RTSP, "[rtsp_ch(%s)] media %d: invalid server ssrc '%s'\n",
				  m_strName.cs(), m_nMediaIndex, strSsrc.cs());
		return EINVAL;
	}

	m_nServerSsrc = (uint32_t)ssrc;

	return ESUCCESS;
}



/*******************************************************************************
 * Debugging support
 */

void CRtspChannel::dump(const char* strPref) const
{
	log_dump("*** %sChannel '%s': id: %u, RTP user/actual port: %u/%u, RTP server port: %u, enabled: %s\n",
			 strPref, m_strName.cs(), m_id, m_nRtpPort, m_nActualRtpPort,
			 m_nServerRtpPort, m_bEnabled ? "YES" : "NO");
	log_dump("    Server SSRC: 0x%X, Media index: %d, URL: '%s'\n",
			 m_nServerSsrc, m_nMediaIndex, m_strUrl.cs());
}
