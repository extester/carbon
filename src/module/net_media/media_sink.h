/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP Video post processor base class
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 24.10.2016 10:22:59
 *		Initial revision.
 */

#ifndef __NET_MEDIA_VIDEO_SINK_H_INCLUDED__
#define __NET_MEDIA_VIDEO_SINK_H_INCLUDED__

#include <list>

#include "shell/counter.h"

#include "carbon/thread.h"
#include "carbon/lock.h"
#include "carbon/carbon.h"

#include "net_media/rtp_playout_buffer.h"

class CVideoSink
{
	protected:
		int 		m_nFps;			/* Video frames per second */
		int 		m_nRate;		/* Timeline clock rate */

	public:
		CVideoSink();
		virtual ~CVideoSink();

	public:
		int getFps() const { return m_nFps; }
		int getClockRate() const { return m_nRate; }

		virtual void put(CRtpPlayoutNode* pNode) = 0;

		virtual result_t init(int nFps, int nRate) {
			shell_assert(nFps);
			m_nFps = nFps;
			m_nRate = nRate;

			return ESUCCESS;
		}

		virtual void terminate() {
		}

		virtual void dump(const char* strPref = "") const = 0;
};

class CVideoSinkV : public CVideoSink, public CThread
{
	private:
		std::list<CRtpPlayoutNode*>	m_arNode;			/* Completed node list */
		mutable CCondition			m_cond;				/* List synchonization/wakeup */
		atomic_t					m_bDone;			/* Cancellation flag */

		counter_t					m_nNodes;			/* DBG: Full processed node count */

	public:
		CVideoSinkV();
		virtual ~CVideoSinkV();

	public:
		virtual void put(CRtpPlayoutNode* pNode);

		virtual result_t init(int nFps, int nRate);
		virtual void terminate();

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual CRtpPlayoutNode* getNextNode();
		virtual void processNode(CRtpPlayoutNode* pNode) = 0;

		virtual void wakeup();
		virtual void clear();

	private:
		void* threadProc(CThread* pThread, void* pData);
};

#endif /* __NET_MEDIA_VIDEO_SINK_H_INCLUDED__ */
