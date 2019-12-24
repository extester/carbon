/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP network receiver pool
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 12.10.2016 10:39:44
 *		Initial revision.
 */
/*
 *				+-------------------+
 * 			  	| CRtpFrameCache	|
 * 				+-------------------+
 *
 * 				+-------------------+			+---------------------------------------+
 * 	  net --+->	| CRtpReceiver		|---+-----> | CRtpInputQueue (in CRtpPlayoutBuffer)	|
 * 			|	+-------------------+	|		+---------------------------------------+
 *			|	 ...					+----->	 ...
 * 			|	 ...					|		+---------------------------------------+
 *	        +->	 ...					+----->	| CRtpInputQueue (in CRtpPlayoutBuffer)	|
 *			|	 ...							+---------------------------------------+
 *			|	 ...
 * 			|	+-------------------+			+---------------------------------------+
 * 			+->	| CRtpReceiver		|---+-----> | CRtpInputQueue (in CRtpPlayoutBuffer)	|
 * 				+-------------------+	|		+---------------------------------------+
 *										+----->	...
 * 										|		+---------------------------------------+
 *										+----->	| CRtpInputQueue (in CRtpPlayoutBuffer)	|
 *												+---------------------------------------+
 */

#ifndef __NET_MEDIA_RTP_RECEIVER_POOL_H_INCLUDED__
#define __NET_MEDIA_RTP_RECEIVER_POOL_H_INCLUDED__

#include <vector>

#include "shell/socket.h"
#include "shell/counter.h"
#include "shell/object.h"

#include "carbon/thread.h"
#include "carbon/lock.h"

#include "net_media/rtp.h"
#include "net_media/rtp_frame_cache.h"

class CRtpPlayoutBuffer;
class CRtpReceiverPool;

class CRtpReceiver : public CThread
{
	private:
		CRtpReceiverPool*	m_pParent;			/* Parent receiver pool */

		mutable CMutex		m_lock;				/* Queue internal lock */
		std::vector<CRtpPlayoutBuffer*> m_arPlayoutBuffer;	/* Playout buffers */

		const CNetAddr		m_netAddr;			/* Network receive address */
		CSocket				m_socket;			/* Network socket */
		atomic_t			m_bDone;			/* Cancellation flag */

		uint16_t			m_nLastSeq;			/* DBG: Last frame seq number or 0xffff */
		counter_t			m_nFrameCount;		/* DBG: Successful received frame count */
		counter_t			m_nFrameDrop;		/* DBG: Dropped frame count */
		counter_t			m_nFrameErr;		/* DBG: Invalid frame count */
		counter_t			m_nFrameErrMem;		/* DBG: Allocation fail count */
		counter_t			m_nFrameLost;		/* DBG: Lost frames */

	public:
		CRtpReceiver(const CNetAddr& netAddr, CRtpReceiverPool* pParent);
		virtual ~CRtpReceiver();

	public:
		result_t init();
		void terminate();

		const CNetAddr& getNetAddr() const { return m_netAddr; }
		size_t getSize() const { CAutoLock locker(m_lock); return m_arPlayoutBuffer.size(); }
		int findChannel(const CRtpPlayoutBuffer* pBuffer, boolean_t bLock = FALSE) const;

		void insertChannel(CRtpPlayoutBuffer* pBuffer);
		result_t removeChannel(const CRtpPlayoutBuffer* pBuffer);
		void removeAllChannels();

		void getStat(int* pnFrameLost) const {
			*pnFrameLost = counter_get(m_nFrameLost);
		}
		void dump(const char* strPref = "") const;

	private:
		result_t validateFrame(rtp_frame_t* pFrame) const;
		void queueFrame(rtp_frame_t* pFrame);

		void* threadProc(CThread* pThread, void* pData);
};

class CRtpReceiverPool : public CObject
{
	private:
		CRtpFrameCache*		m_pFrameCache;		/* Common frame cache */
		mutable CMutex		m_lock;				/* Pool internal lock */
		std::vector<CRtpReceiver*>	m_arReceiver;	/* Receivers list */
		boolean_t			m_bReceiving;		/* TRUE: poll is receiving */

	public:
		CRtpReceiverPool(const char* strName, int nMaxCacheFrames = RTP_FRAME_CACHE_LIMIT);
		virtual ~CRtpReceiverPool();

	public:
		result_t init();
		void terminate();
		void reset() {
			if ( m_pFrameCache )  {
				m_pFrameCache->clear();
			}
		}
		boolean_t isReceiving() const { CAutoLock locker(m_lock); return m_bReceiving; }

		void insertChannel(CRtpPlayoutBuffer* pBuffer, const CNetAddr& netAddr);
		void removeChannel(const CRtpPlayoutBuffer* pBuffer);
		void removeAllChannels();

		rtp_frame_t* getCacheFrame() { return m_pFrameCache->get(); }

		void getStat(const CRtpPlayoutBuffer* pBuffer, int* pnFrameLost) const;
		void dump(const char* strPref = "") const;

	private:
		int findChannel(const CNetAddr& netAddr) const;
		int findChannel(const CRtpPlayoutBuffer* pBuffer) const;
};

#endif /* __NET_MEDIA_RTP_RECEIVER_POOL_H_INCLUDED__ */
