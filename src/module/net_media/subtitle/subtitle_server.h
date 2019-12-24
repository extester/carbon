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
 *  Revision 1.0, 28.02.2017 13:01:46
 *      Initial revision.
 */

#ifndef __NET_MEDIA_SUBTITLE_SERVER_H_INCLUDED__
#define __NET_MEDIA_SUBTITLE_SERVER_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/timer.h"
#include "carbon/event/eventloop.h"

#include "net_media/subtitle/subtitle_frame.h"
#include "net_media/subtitle/subtitle_sink.h"

class CSubtitleServer
{
	protected:
		CSubtitleFrame*			m_pFrame;		/* Subtitle frame for I/O */

		const unsigned int 		m_nClockRate;	/* Sample rate, i.e 1000 */
		const unsigned int		m_nInterval;	/* Subtitle generation interval */

		uint64_t				m_timestampNext;/* Next frame timestamp */

		CSubtitleSinkS*			m_pSink;		/* Subtitle sink */
		CEventLoop*				m_pEventLoop;	/* Owner event loop */
		CTimer*					m_pTimer;		/* Frame generation time */

		counter_t				m_nFrames;		/* DBG: Generated frame count */

	public:
		CSubtitleServer(CEventLoop* pEventLoop, unsigned int nClockRate,
						unsigned int nInterval, CSubtitleSinkS* pSink);
		virtual ~CSubtitleServer();

	public:
		virtual void start();
		virtual void stop();

		virtual result_t init();
		virtual void terminate();

		virtual void dump(const char* strPref = "") const;

	protected:
		void stopTimer();
		virtual void generateFrame(void* p);
		virtual void formatFrame(uint64_t timestamp, hr_time_t hrPts, CSubtitleFrame* pFrame) = 0;
};


#endif /* __NET_MEDIA_SUBTITLE_SERVER_H_INCLUDED__ */
