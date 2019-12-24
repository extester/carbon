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
 *	Revision 1.0, 10.11.2016 18:12:33
 *		Initial revision.
 */

#include "net_media/rtp_session.h"

/*******************************************************************************
 * CRtpSession class
 */

CRtpSession::CRtpSession(uint32_t ssrc, rtp_source_t* pSource, int rate) :
	m_ssrc(ssrc),
	m_pSource(pSource),
	m_rate(rate),
	m_lsr(0),
	m_hrLsr(HR_0)
{
}

CRtpSession::~CRtpSession()
{
}

void CRtpSession::onRtpFrame(const rtp_frame_t* pFrame)
{
	int 		transit, d;
	hr_time_t	hrArrival;

	hrArrival = (pFrame->hrArriveTime-m_pSource->hrTimelineBase)*m_rate/HR_1SEC;

	transit = (int)((uint32_t)hrArrival - pFrame->head.timestamp);
	d = transit - m_pSource->transit;
	m_pSource->transit = transit;
	if ( d < 0 )  d = -d;
	m_pSource->jitter += (1./16.) * ((double)d - m_pSource->jitter);
}

void CRtpSession::onRtcpSrPacket(const rtcp_sr_packet_t* pPacket)
{
	m_lsr = (uint32_t)((pPacket->ntp_timestamp >> 16)&0xffffffff);
	m_hrLsr = hr_time_now();
}


void CRtpSession::getData(rtcp_report_block_t* pBlock)
{
	uint32_t	expected, expected_interval, received_interval,
				lost_interval, fraction;
	int32_t		lost;

	/* highSequence */
	pBlock->highSequence = (uint32_t)m_pSource->cycles + m_pSource->max_seq;

	/* Cumulative number of packets lost */
	expected = pBlock->highSequence - m_pSource->base_seq + 1;
	lost = expected - m_pSource->received;
	if ( lost > 0 )  {
		lost &= 0x7fffff;
	}
	else {
		if ( lost < -0x7f0000 ) {
			lost = 0x800000;
		}
		lost &= 0xffffff;
	}

	/* Loss fraction */
	expected_interval = expected - m_pSource->expected_prior;
	m_pSource->expected_prior = expected;
	received_interval = m_pSource->received - m_pSource->received_prior;
	m_pSource->received_prior = m_pSource->received;

	lost_interval = expected_interval - received_interval;
	if ( expected_interval == 0 || lost_interval <= 0 )  {
		fraction = 0;
	}
	else {
		fraction = (lost_interval << 8) / expected_interval;
	}

	pBlock->lost = RTCP_REPORT_BLOCK_FORMAT_LOST(lost, fraction&0xff);

	/* Jitter */
	pBlock->jitter = (uint32_t)m_pSource->jitter;

	/* LSR */
	pBlock->lsr = m_lsr;

	/* DLSR */
	if ( m_hrLsr != HR_0 )  {
		pBlock->dlsr = (uint32_t)((hr_time_now()-m_hrLsr) * 65536 / HR_1SEC);
	}
	else {
		pBlock->dlsr = 0;
	}
}


/*******************************************************************************
 * CRtpSessionManager class
 */

#define RTP_SESIONS_MAX		20

CRtpSessionManager::CRtpSessionManager()
{
}

CRtpSessionManager::~CRtpSessionManager()
{
	while ( !m_session.empty() )  {
		delete m_session.begin()->second;
		m_session.erase(m_session.begin());
	}
}

/*
 * Create a new RTP session
 *
 * 		ssrc		new session SSRC (must be unique)
 *
 * Return: new inserted session object pointer or 0
 *
 * Note: manager's map must be locked
 */
CRtpSession* CRtpSessionManager::createSession(uint32_t ssrc, rtp_source_t* pSource, int rate)
{
	CRtpSession*	pSession;
	std::pair<std::map<uint32_t, CRtpSession*>::iterator, bool>		result;

	if ( m_session.size() >= RTP_SESIONS_MAX )  {
		return 0;
	}

	pSession = new CRtpSession(ssrc, pSource, rate);
	result = m_session.insert(std::pair<uint32_t, CRtpSession*>(ssrc, pSession));
	if ( result.second == false )  {
		shell_assert(FALSE);
		delete pSession;
		pSession = result.first->second;
	}

	return pSession;
}

/*
 * Delete session
 *
 * 		ssrc		SSRC sesion to delete
 *
 * Note: manager's map must be locked
 */
void CRtpSessionManager::deleteSession(uint32_t ssrc)
{
	std::map<uint32_t, CRtpSession*>::iterator	it;

	it = m_session.find(ssrc);
	if ( it != m_session.end() )  {
		delete it->second;
		m_session.erase(it);
	}
}

void CRtpSessionManager::onRtpFrame(const rtp_frame_t* pFrame, rtp_source_t* pSource, int rate)
{
	CAutoLock		locker(m_lock);
	std::map<uint32_t, CRtpSession*>::iterator	it;
	CRtpSession*	pSession;

	it = m_session.find(pFrame->head.ssrc);
	if ( it != m_session.end() )  {
		pSession = it->second;
	}
	else {
		pSession = createSession(pFrame->head.ssrc, pSource, rate);
		if ( pSession == 0 )  {
			return;
		}
	}

	pSession->onRtpFrame(pFrame);
}

void CRtpSessionManager::onRtcpSrPacket(const rtcp_sr_packet_t* pPacket)
{
	CAutoLock		locker(m_lock);
	std::map<uint32_t, CRtpSession*>::iterator	it;

	it = m_session.find(pPacket->ssrc);
	if ( it != m_session.end() )  {
		it->second->onRtcpSrPacket(pPacket);
	}
}

void CRtpSessionManager::getSessionData(uint32_t ssrc, rtcp_report_block_t* pBlock)
{
	CAutoLock		locker(m_lock);
	std::map<uint32_t, CRtpSession*>::iterator	it;

	it = m_session.find(ssrc);
	if ( it != m_session.end() ) {
		it->second->getData(pBlock);
	}
	else {
		_tbzero(pBlock, sizeof(*pBlock));
	}
}

void CRtpSessionManager::dump(const char* strPref) const
{
	CAutoLock	locket(m_lock);
	char 		strBuf[256] = "";
	int 		len = 0;
	std::map<uint32_t, CRtpSession*>::const_iterator	it;

	for(it=m_session.begin(); it != m_session.end(); it++) {
		if ( len+1 >= (int)sizeof(strBuf) ) {
			break;
		}

		len += _tsnprintf(&strBuf[len], sizeof(strBuf)-len, "%s 0x%X",
				it != m_session.begin() ? "," : "", it->first);
	}

	log_dump("*** %sSessionManager(%u): %s\n", strPref, m_session.size(), strBuf);
}
