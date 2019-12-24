/*
 *	Carbon/Network MultiMedia Streaming Module
 *  Audio Server Base class
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 21.11.2016 16:37:59
 *      Initial revision.
 */

#ifndef __NET_MEDIA_AUDIO_SERVER_H_INCLUDED__
#define __NET_MEDIA_AUDIO_SERVER_H_INCLUDED__

#include "shell/counter.h"

#include "carbon/carbon.h"
#include "carbon/thread.h"

#include "net_media/audio/audio_frame.h"
#include "net_media/audio/audio_sink.h"

class CAudioServer
{
	protected:
		std::list<CAudioFrame*>	m_frames;		/* Free frame list */
		mutable CCondition		m_cond;			/* Synchronisation/Sleep/Wakeup */

		const unsigned int 		m_nFormat;		/* Sample format */
		const unsigned int 		m_nChannels;	/* Number of channels: 1: mono, 2: stereo */
		const unsigned int 		m_nClockRate;	/* Sample rate, i.e 16000 */

		CAudioSink*				m_pSink;

		counter_t				m_nFrames;		/* DBG: Full number of allocated frames */
		counter_t				m_nCurFrames;	/* DBG: Currently allocated frames */
		counter_t				m_nMaxFrames;	/* DBG: Max simultaneous allocated frames */
		counter_t				m_nOverFrames;	/* DBG: Out of frame errors */

	public:
		CAudioServer(unsigned int nFormat, unsigned int nChannels,
					 unsigned int nClockRate, CAudioSink* pSink);
		virtual ~CAudioServer();

	public:
		void putFrame(CAudioFrame* pFrame);

		virtual void start() = 0;
		virtual void stop() = 0;

		virtual result_t init();
		virtual void terminate();

		virtual void dump(const char* strPref = "") const;

	protected:
		CAudioFrame* getFrame();
		virtual void allocFrames(size_t nBufferSize) = 0;
		virtual void freeFrames();
};


#endif /* __NET_MEDIA_AUDIO_SERVER_H_INCLUDED__ */
