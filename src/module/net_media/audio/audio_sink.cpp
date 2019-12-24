/*
 *	Carbon/Network MultiMedia Streaming Module
 *	Audio frame post processor
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 22.11.2016 18:12:46
 *		Initial revision.
 */

#include "carbon/carbon.h"

#include "net_media/audio/audio_frame.h"
#include "net_media/audio/audio_sink.h"

#define AUDIO_SINKA_IDLE				HR_13SEC

/*******************************************************************************
 * CAudioSink class
 */

CAudioSink::CAudioSink() :
	m_nSamplesPerFrame(0),
	m_nClockRate(0),
	m_pDecoderInfo(0),
	m_nDecoderInfoSize(0)
{
}

CAudioSink::~CAudioSink()
{
}

/*******************************************************************************
 * CAudioSinkA class
 */

CAudioSinkA::CAudioSinkA() :
	CAudioSink(),
	m_worker("AudioSinkA", HR_0, HR_4SEC)
{
}

CAudioSinkA::~CAudioSinkA()
{
}

/*
 * Wake up a sink thread to process outstanding frames
 */
void CAudioSinkA::wakeup()
{
	m_cond.lock();
	m_cond.wakeup();
	m_cond.unlock();
}

/*
 * Remove all awaiting frames from the list
 */
void CAudioSinkA::clear()
{
	CAutoLock		locker(m_cond);
	CAudioFrame*	pFrame;

	while ( !m_frames.empty() ) {
		pFrame = *m_frames.begin();
		m_frames.pop_front();
		locker.unlock();
		pFrame->put();
	}
}

/*
 * Free audio frame and put it to available frames list
 *
 * 		pFrame		frame to put
 */
void CAudioSinkA::put(CAudioFrame* pFrame)
{
	CAutoLock	locker(m_cond);

	if ( atomic_get(&m_bDone) == FALSE ) {
		m_frames.push_back(pFrame);
		counter_inc(m_nFrames);
		m_cond.wakeup();
	}
}

/*
 * Pick up next available frame for processing
 *
 * Return: frame pointer or AUDIO_FRAME_NULL if no complete nodes
 */
CAudioFrame* CAudioSinkA::getNextFrame()
{
	CAutoLock		locker(m_cond);
	CAudioFrame*	pFrame = AUDIO_FRAME_NULL;

	if ( !m_frames.empty() )  {
		pFrame = *m_frames.begin();
		m_frames.pop_front();
	}

	return pFrame;
}

/*
 * Sink worker thread
 */
void* CAudioSinkA::worker(CThread* pThread, void* pData)
{
	CAudioFrame*	pFrame;

	while ( atomic_get(&m_bDone) == FALSE )  {
		pFrame = getNextFrame();
		if ( pFrame )  {
			processFrame(pFrame);
			pFrame->put();
		}

		m_cond.lock();
		if ( atomic_get(&m_bDone) == FALSE && m_frames.size() == 0 )  {
			m_cond.waitTimed(hr_time_now()+AUDIO_SINKA_IDLE);
		}
		m_cond.unlock();
	}

	return NULL;
}

/*
 * Initialise and start receiving an audio frames
 *
 * 		nSamplesPerFrame		Audio samples per frame
 * 		nClockRate				Audio timeline clock rate
 * 		pDecoderInfo			Audio decoder information
 * 		nDecoderInfoSize		Audio decoder info size
 *
 * Return: ESUCCESS, ...
 */
result_t CAudioSinkA::init(unsigned int nSamplesPerFrame, unsigned int nClockRate,
							  const void* pDecoderInfo, size_t nDecoderInfoSize)
{
	result_t	nresult;

	log_debug(L_GEN, "[audio_sinka] init sink: spf: %d, rate: %d, decoder info: %d byte(s)\n",
			  nSamplesPerFrame, nClockRate, nDecoderInfoSize);

	nresult = CAudioSink::init(nSamplesPerFrame, nClockRate, pDecoderInfo, nDecoderInfoSize);
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	atomic_set(&m_bDone, FALSE);

	nresult = m_worker.start(THREAD_CALLBACK(CAudioSinkA::worker, this), 0);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[audio_sinka] failed to start worker thread, result: %d\n", nresult);
		CAudioSink::terminate();
	}

	return nresult;
}

/*
 * Deinitialise and terminate audio sink
 */
void CAudioSinkA::terminate()
{
log_debug(L_GEN, "[deca] -t1-\n");
	atomic_set(&m_bDone, TRUE);
	wakeup();
log_debug(L_GEN, "[deca] -t2-\n");
	m_worker.stop();
log_debug(L_GEN, "[deca] -t3-\n");
	clear();

	CAudioSink::terminate();
}

/*******************************************************************************
 * Debugging support
 */

void CAudioSinkA::dump(const char* strPref) const
{
	CAutoLock	locker(m_cond);

	log_dump("*** %sAudioSinkA: pending frames: %u, full frame count: %u\n",
			 strPref, m_frames.size(), counter_get(m_nFrames));
}
