/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP input queue
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 12.10.2016 12:18:25
 *		Initial revision.
 */

#include "net_media/h264.h"
#include "net_media/rtp_frame_cache.h"
#include "net_media/rtp_playout_buffer.h"
#include "net_media/rtp_input_queue.h"


/*******************************************************************************
 * CRtpInputQueue class
 */

CRtpInputQueue::CRtpInputQueue(int nMaxLength) :
	m_nLength(0),
	m_nMaxLength(nMaxLength),
	m_hrMinInterval(HR_FOREVER),
	m_hrMaxInterval(HR_0),
	m_hrPrevAriveTime(HR_0)
{
	queue_init(&m_queue);

	counter_init(m_nFullFrames);
	counter_init(m_nMaxFrames);
	counter_init(m_nOverFrames);
}

CRtpInputQueue::~CRtpInputQueue()
{
	shell_assert(m_nLength == 0);
}

/*
 * Remove all frames out of queue
 */
void CRtpInputQueue::clear()
{
	CAutoLock		locker(m_lock);
	rtp_frame_t*	pFrame;

	while ( !queue_empty(&m_queue) )  {
		queue_remove_first(&m_queue, pFrame, rtp_frame_t*, link);
		pFrame->pOwner->put(pFrame);
	}

	m_nLength = 0;
}

/*
 * Put frame to queue
 *
 * 		pFrame		frame to put to queue
 */
void CRtpInputQueue::put(rtp_frame_t* pFrame)
{
	CAutoLock	locker(m_lock);

	if ( m_nLength >= m_nMaxLength )  {
		counter_inc(m_nOverFrames);
		pFrame->pOwner->put(pFrame);
		log_error(L_GEN, "[rtp_inputq] input queue overflow (max %d frames)\n", m_nMaxFrames);
		return;
	}

	queue_enter(&m_queue, pFrame, rtp_frame_t*, link);
	m_nLength++;

	/* Statistics */
	counter_inc(m_nFullFrames);
	if ( m_nLength > counter_get(m_nMaxFrames) )  {
		counter_set(m_nMaxFrames, m_nLength);
	}

	/* Min/Max frame intervals */
	if ( m_hrPrevAriveTime != HR_0 )  {
		hr_time_t	hrInterval = pFrame->hrArriveTime - m_hrPrevAriveTime;

		if ( hrInterval < m_hrMinInterval )  {
			m_hrMinInterval = hrInterval;
		}
		if ( hrInterval > m_hrMaxInterval )  {
			m_hrMaxInterval = hrInterval;
		}
	}
	m_hrPrevAriveTime = pFrame->hrArriveTime;
}

/*
 * Get frame out of queue
 *
 * Return: frame pointer or RTP_FRAME_NULL if queue is empty
 */
rtp_frame_t* CRtpInputQueue::get()
{
	CAutoLock		locker(m_lock);
	rtp_frame_t*	pFrame = RTP_FRAME_NULL;

	if ( m_nLength > 0 )  {
		queue_remove_first(&m_queue, pFrame, rtp_frame_t*, link);
		m_nLength--;
	}

	return pFrame;
}

/*******************************************************************************
 * Debugging support
 */

void CRtpInputQueue::dump(const char* strPref) const
{
	CAutoLock	locker(m_lock);

	log_dump("  > %sRTP Input Queue: frames: %d full, %d current, %d max, %d overflow\n",
			 strPref, counter_get(m_nFullFrames), m_nLength,
			 counter_get(m_nMaxFrames), counter_get(m_nOverFrames));

	log_dump("    Interval: min %u.%03u ms, max %u ms\n",
			 	HR_TIME_TO_MILLISECONDS(m_hrMinInterval),
			 	HR_TIME_TO_MICROSECONDS(m_hrMinInterval)%1000,
			 	HR_TIME_TO_MILLISECONDS(m_hrMaxInterval));
}
