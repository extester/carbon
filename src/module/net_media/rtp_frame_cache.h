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

#ifndef __NET_MEDIA_RTP_FRAME_CACHE_H_INCLUDED__
#define __NET_MEDIA_RTP_FRAME_CACHE_H_INCLUDED__

#include "shell/queue.h"

#include "carbon/carbon.h"
#include "carbon/lock.h"

#include "net_media/rtp.h"

#define RTP_FRAME_CACHE_LIMIT			1000	/* Default maximum allocated frames */

class CRtpFrameCache
{
	private:
		queue_head_t		m_queue;			/* Available free RTP frames */
		int					m_nLength;			/* Available frame count */
		const int 			m_nMaxLength;		/* Maximum allocated frames */
		mutable CMutex		m_lock;				/* Access synchronisation */

		uint32_t			m_hitCount;			/* DBG: Reused frames */
		uint32_t			m_misCount;			/* DBG: Allocated frames */

	public:
		CRtpFrameCache(int nMaxLength = RTP_FRAME_CACHE_LIMIT);
		~CRtpFrameCache();

		void clear() {
			CAutoLock	locker(m_lock);
			queueFree(m_nLength);
		}

		rtp_frame_t* get();
		void put(rtp_frame_t* pFrame);

		void dump(const char* strPref = "") const;

	private:
		void queueFree(int count);

};

#endif /* __NET_MEDIA_RTP_FRAME_CACHE_H_INCLUDED__ */
