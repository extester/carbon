/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP Playout Buffer
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 14.10.2016 17:39:38
 *		Initial revision.
 */

#ifndef __NET_MEDIA_RTP_PLAYOUT_BUFFER_H_INCLUDED__
#define __NET_MEDIA_RTP_PLAYOUT_BUFFER_H_INCLUDED__

#include <list>

#include "shell/queue.h"
#include "shell/counter.h"

#include "carbon/thread.h"
#include "carbon/lock.h"

#include "net_media/rtp.h"
#include "net_media/media_frame.h"

class CRtpInputQueue;
class CVideoSink;
class CRtpPlayoutBuffer;
class CRtpSessionManager;

class CRtpPlayoutNode : public CMediaFrame
{
	protected:
		CRtpPlayoutBuffer*	m_pParent;			/* Parent Playout Buffer */

		queue_head_t		m_queue;			/* Received frames, RTP timestamp ordered */
		int 				m_nLength;			/* Frame count */

		uint64_t			m_rtpRealTimestamp;	/* Real RTP timestamp (no overflow) */
		const hr_time_t		m_hrCreationTime;	/* Node creation time */
		hr_time_t			m_hrPlayoutTime;	/* Playout time (time to send to decoder) */
		hr_time_t			m_hrPts;			/* Presentation time (PTS) (available after completeNode() */

		int 				m_nCurDelay;		/* Current extra delay time */

	public:
		CRtpPlayoutNode(uint64_t rtpRealTimestamp, hr_time_t hrPlayoutTime, CRtpPlayoutBuffer* pParent);
	protected:
		virtual ~CRtpPlayoutNode();

	public:
		virtual uint64_t getTimestamp() const { return m_rtpRealTimestamp; }
		virtual void setTimestamp(uint64_t timestamp) { m_rtpRealTimestamp = timestamp; }

		virtual hr_time_t getPts() const { return m_hrPts; }
		hr_time_t getPlayoutTime() const { return m_hrPlayoutTime; }
		hr_time_t getCreationTime() const { return m_hrCreationTime; }

		virtual result_t insertFrame(rtp_frame_t* pFrame);
		virtual boolean_t isReady() const = 0;
		int incrDelayCount() { int value = m_nCurDelay; m_nCurDelay++; return value; }
		void addPlayoutTime(hr_time_t hrDelta) { m_hrPlayoutTime += hrDelta; }

		virtual uint8_t* getData() = 0;
		virtual size_t getSize() const = 0;

		int validateSeqOrder() const;
		int validateTimestamp() const;
		virtual void dump(const char* strPref = "") const;

	protected:
		virtual void clear();
};

class CRtpPlayoutBuffer : public CThread
{
	protected:
		CRtpInputQueue*		m_pInputQueue;			/* Source flame queue */
		CVideoSink*			m_pSink;				/* Post processor */

		std::list<CRtpPlayoutNode*>		m_arNode;	/* Node list */
		CCondition			m_cond;
		atomic_t			m_bDone;				/* Process stopped? */

		const int 			m_nProfile;				/* Media profile, i.e 96 */
		const int 			m_nFps;					/* Frames per second */
		const int			m_nClockRate;			/* Clock rate */
		rtp_source_t		m_rtpSource;			/* RTP proto parameters */
		boolean_t			m_rtpSourceInited;		/* RTP proto parameters flag */

		uint64_t			m_lastPlayoutTimestamp;	/* Last playout node timestamp */

		CRtpSessionManager*	m_pSessionManager;

		const int 			m_nMaxDelay;			/* Awaiting node frames extra time
 													 * in (m_nClockRate/m_nFps)/2 */

		counter_t			m_nFrameCount;			/* DBG: Full frame count */
		counter_t			m_nFrameDropped;		/* DBG: Frame dropped count */
		counter_t			m_nFrameLate;			/* DBG: Frame late (dropped) */
		counter_t			m_nNodeDropped;			/* DBG: Node dropped count */
		counter_t			m_nNodeMaxCount;		/* DBG: Max nodes in queue */

	public:
		CRtpPlayoutBuffer(int nProfile, int nFps, int nClockRate, int nMaxInputQueue,
						  	int nMaxDelay, const char* strName);
		virtual ~CRtpPlayoutBuffer();

	public:
		int getProfile() const { return m_nProfile; }
		int getFps() const { return m_nFps; }
		int getClockRate() const { return m_nClockRate; }

		virtual result_t init(CVideoSink* pSink);
		virtual void terminate();
		void setSessionManager(CRtpSessionManager* pSessionMan) { m_pSessionManager = pSessionMan; }

		void putFrame(rtp_frame_t* pFrame);
		void flush() {
			playout();
		}

		virtual void dump(const char* strPref = "") const;
		void dumpNodes() const;
		int validateNodeOrder() const;
		void getFrameStat(int* pnNodeDropped) const
		{
			*pnNodeDropped = counter_get(m_nNodeDropped);
		}

	protected:
		virtual CRtpPlayoutNode* createNode(rtp_frame_t* pFrame, uint64_t rtpRealTimestamp) = 0;
		virtual void clear();

	private:
		void wakeup();
		void getInputFrames();
		void playout();
		hr_time_t getNextWakeupTime() const;
		void insertNode(CRtpPlayoutNode* pNode);
		CRtpPlayoutNode* findNode(uint64_t rtpRealTimestamp) const;

		void* threadProc(CThread* pThread, void* pData);
};

#endif /* __NET_MEDIA_RTP_PLAYOUT_BUFFER_H_INCLUDED__ */
