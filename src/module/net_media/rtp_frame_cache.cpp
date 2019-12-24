/*
 *	Carbon/Network MultiMedia Streaming Module
 *	Preallocate RTP frame cache
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 11.10.2016 17:41:00
 *		Initial revision.
 */

#include "carbon/memory.h"

#include "net_media/rtp_frame_cache.h"

/*******************************************************************************
 * CRtpFrameCache class
 */

CRtpFrameCache::CRtpFrameCache(int nMaxLength) :
	m_nLength(0),
	m_nMaxLength(nMaxLength),
	m_hitCount(0),
	m_misCount(0)
{
	queue_init(&m_queue);
}

CRtpFrameCache::~CRtpFrameCache()
{
	clear();
}

/*
 * Allocate a RTP frame from the cache
 *
 * Return: frame pointer or RTP_FRAME_NULL
 */
rtp_frame_t* CRtpFrameCache::get()
{
	CAutoLock		locker(m_lock);
	rtp_frame_t*	pFrame;

	if ( m_nLength > 0 )  {
		queue_remove_first(&m_queue, pFrame, rtp_frame_t*, link);
		shell_assert(pFrame->pOwner == this);
		m_nLength--;
		m_hitCount++;
	}
	else {
		m_misCount++;

		if ( m_nLength < m_nMaxLength ) {
			locker.unlock();
			pFrame = (rtp_frame_t*)memAlloc(sizeof(rtp_frame_t));
			if ( pFrame != RTP_FRAME_NULL ) {
				pFrame->pOwner = this;
			}
		}
		else {
			/* Allocated frames limit reached */
			pFrame = RTP_FRAME_NULL;
		}
	}

	return pFrame;
}

/*
 * Put an RTP frame to the cache
 *
 * 		pFrame			frame to put back to the cache
 */
void CRtpFrameCache::put(rtp_frame_t* pFrame)
{
	CAutoLock		locker(m_lock);

	if ( pFrame != RTP_FRAME_NULL )  {
		queue_enter_first(&m_queue, pFrame, rtp_frame_t*, link);
		m_nLength++;
	}
}

/*
 * Free cached frames
 *
 * 		count		frames to free
 *
 * Note: queue lock must be held
 */
void CRtpFrameCache::queueFree(int count)
{
	rtp_frame_t*	pFrame;
	int 			n = 0;

	while ( !queue_empty(&m_queue) && n < count )  {
		queue_remove_first(&m_queue, pFrame, rtp_frame_t*, link);
		m_nLength--;
		memFree(pFrame);
	}
}

/*******************************************************************************
 * Debugging support
 */

void CRtpFrameCache::dump(const char* strPref) const
{
	CAutoLock	locker(m_lock);
	int 		hit = 0;

	if ( m_hitCount+m_misCount != 0 ) {
		hit = (int)((m_hitCount * 100) / (m_hitCount + m_misCount));
	}

	log_dump("*** %sRTP Frame Cache: hit ratio %d%% (hit: %d, mis: %d)\n",
				strPref, hit, m_hitCount, m_misCount);
}
