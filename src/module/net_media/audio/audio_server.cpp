/*
 *	Carbon/Network MultiMedia Streaming Module
 *  Audio Server Base class
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 21.11.2016 16:41:55
 *      Initial revision.
 */

#include "net_media/audio/audio_server.h"

/*******************************************************************************
 * CAudioServer class
 */

CAudioServer::CAudioServer(unsigned int nFormat, unsigned int nChannels,
						   unsigned int nClockRate, CAudioSink* pSink) :
	m_nFormat(nFormat),
	m_nChannels(nChannels),
	m_nClockRate(nClockRate),
	m_pSink(pSink),

	m_nFrames(ZERO_COUNTER),
	m_nCurFrames(ZERO_COUNTER),
	m_nMaxFrames(ZERO_COUNTER),
	m_nOverFrames(ZERO_COUNTER)
{
	shell_assert(nClockRate);
	shell_assert(nChannels == 1 || nChannels == 2);
}

CAudioServer::~CAudioServer()
{
	shell_assert(m_frames.size() == 0);
}

/*
 * Free frame list
 */
void CAudioServer::freeFrames()
{
	std::list<CAudioFrame*>::iterator iter = m_frames.begin();

	while ( iter != m_frames.end() )  {
		(*iter)->release();
		iter++;
	}

	m_frames.clear();
}

CAudioFrame* CAudioServer::getFrame()
{
	CAutoLock		locker(m_cond);
	CAudioFrame* 	pFrame = AUDIO_FRAME_NULL;

	if ( !m_frames.empty() )  {
		pFrame = m_frames.front();
		m_frames.pop_front();

		counter_inc(m_nFrames);
		counter_inc(m_nCurFrames);
		if ( counter_get(m_nCurFrames) > counter_get(m_nMaxFrames) )  {
			counter_set(m_nMaxFrames, counter_get(m_nCurFrames));
		}
	}
	else {
		counter_inc(m_nOverFrames);
	}

	return pFrame;
}

void CAudioServer::putFrame(CAudioFrame* pFrame)
{
	if ( pFrame ) {
		CAutoLock		locker(m_cond);

		m_frames.push_back(pFrame);

		counter_dec(m_nCurFrames);
		shell_assert(counter_get(m_nCurFrames) >= 0);
	}
}

result_t CAudioServer::init()
{
	result_t	nresult = ESUCCESS;

	return nresult;
}

void CAudioServer::terminate()
{
}

/*******************************************************************************
 * Debugging support
 */

void CAudioServer::dump(const char* strPref) const
{
	log_dump("*** %sAudioServer: format: %d, channels: %d, clock rate: %d\n",
			 strPref, m_nFormat, m_nChannels, m_nClockRate);

	log_dump("    Frames: %d, cur.frames: %d, max.frames: %d, overflow: %d\n",
			 counter_get(m_nFrames), counter_get(m_nCurFrames),
			 counter_get(m_nMaxFrames), counter_get(m_nOverFrames));
}
