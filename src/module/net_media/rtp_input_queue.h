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
 *	Revision 1.0, 12.10.2016 12:16:46
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTP_INPUT_QUEUE_H_INCLUDED__
#define __NET_MEDIA_RTP_INPUT_QUEUE_H_INCLUDED__

#include "shell/queue.h"
#include "shell/counter.h"

#include "carbon/lock.h"

#include "net_media/rtp.h"

class CRtpInputQueue
{
	protected:
		queue_head_t		m_queue;				/* Frame queue */
		int					m_nLength;				/* Frame queue length */
		const int 			m_nMaxLength;			/* Frame queue length limit */
		mutable CMutex		m_lock;					/* Frame queue lock */

		counter_t			m_nFullFrames;			/* DBG: Full frame count */
		counter_t			m_nMaxFrames;			/* DBG: Maximum queue length */
		counter_t			m_nOverFrames;			/* DBG: overflow frames */

		hr_time_t			m_hrMinInterval;		/* DBG: Minimal frame interval or HR_0 */
		hr_time_t			m_hrMaxInterval;		/* DBG: Maximum frame interval or HR_0 */
		hr_time_t			m_hrPrevAriveTime;		/* DBG: Previous frame time */

	public:
		explicit CRtpInputQueue(int nMaxLength);
		~CRtpInputQueue();

	public:
		void clear();
		int getLength() const { CAutoLock locker(m_lock); return m_nLength; }

		void put(rtp_frame_t* pFrame);
		rtp_frame_t* get();

		void dump(const char* strPref = "") const;
};

#endif /* __NET_MEDIA_RTP_INPUT_QUEUE_H_INCLUDED__ */
