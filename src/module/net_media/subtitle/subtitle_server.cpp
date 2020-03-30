/*
 *	Carbon/Network MultiMedia Streaming Module
 *  Subtitle Server Base class
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 28.02.2017 13:18:43
 *      Initial revision.
 */

#include "net_media/subtitle/subtitle_server.h"


/*******************************************************************************
 * CSubtitleServer class
 */

CSubtitleServer::CSubtitleServer(CEventLoop* pEventLoop, unsigned int nClockRate,
								 unsigned int nInterval, CSubtitleSinkS* pSink) :
	m_nClockRate(nClockRate),
	m_nInterval(nInterval),
	m_pSink(pSink),
	m_pEventLoop(pEventLoop),
	m_pTimer(0),
	m_nFrames(ZERO_COUNTER)
{
	shell_assert(nClockRate);
	shell_assert(nInterval != HR_0);
	shell_assert(pSink);

	m_pFrame = new CSubtitleFrame(this);
	m_timestampNext = nInterval;
}

CSubtitleServer::~CSubtitleServer()
{
	m_pFrame->release();
}

/*
 * Generate and process a single subtitle frame
 */
void CSubtitleServer::generateFrame(void* p)
{
	shell_unused(p);

	formatFrame(m_timestampNext, hr_time_now(), m_pFrame);
	m_pSink->processFrame(m_pFrame);
	m_timestampNext += m_nInterval;
	counter_inc(m_nFrames);
}

/*
 * Stop and delete frame generation timer
 */
void CSubtitleServer::stopTimer()
{
	SAFE_DELETE_TIMER(m_pTimer, m_pEventLoop);
}


/*
 * Start generating subtitle frames
 */
void CSubtitleServer::start()
{
	hr_time_t	hrInterval;

	shell_assert(m_pTimer == 0);
	stopTimer();

	hrInterval = HR_1SEC*m_nInterval/m_nClockRate;

	m_pTimer = new CTimer(hrInterval, TIMER_CALLBACK(CSubtitleServer::generateFrame, this),
						  	CTimer::timerPeriodic, "subtitle-server");
	m_pEventLoop->insertTimer(m_pTimer);
}

/*
 * Stop generation subtitle frames
 */
void CSubtitleServer::stop()
{
	stopTimer();
}

/*
 * Initialise server
 *
 * Return: ESUCCESS, ...
 */
result_t CSubtitleServer::init()
{
	result_t	nresult = ESUCCESS;

	counter_init(m_nFrames);

	return nresult;
}

/*
 * Release server resources
 */
void CSubtitleServer::terminate()
{
	stop();
}

/*******************************************************************************
 * Debugging support
 */

void CSubtitleServer::dump(const char* strPref) const
{
	log_dump("*** %sSubtitleServer: clock rate: %d, interval %u, frames %u\n",
			 strPref, m_nClockRate, m_nInterval, counter_get(m_nFrames));
}
