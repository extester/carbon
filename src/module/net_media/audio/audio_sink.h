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
 *	Revision 1.0, 22.11.2016 18:10:24
 *		Initial revision.
 */

#ifndef __NET_MEDIA_AUDIO_SINK_H_INCLUDED__
#define __NET_MEDIA_AUDIO_SINK_H_INCLUDED__

#include <list>

#include "shell/atomic.h"
#include "shell/counter.h"

#include "carbon/thread.h"

#include "net_media/audio/audio_frame.h"

/*
 * Base class for audio frames processor
 */
class CAudioSink
{
	protected:
		unsigned int		m_nSamplesPerFrame;		/* Audio samples per frame */
		unsigned int 		m_nClockRate;			/* Audio timeline clock rate */
		const void*			m_pDecoderInfo;			/* Audio decoder info */
		size_t				m_nDecoderInfoSize;		/* Audio decoder info size */

	public:
		CAudioSink();
		virtual ~CAudioSink();

	public:
		int getClockRate() const { return m_nClockRate; }
		virtual void put(CAudioFrame* pFrame) = 0;

		virtual result_t init(unsigned int nSamplesPerFrame, unsigned int nClockRate,
							  const void* pDecoderInfo, size_t nDecoderInfoSize) {
			shell_assert(nSamplesPerFrame);
			shell_assert(nClockRate);
			m_nSamplesPerFrame = nSamplesPerFrame;
			m_nClockRate = nClockRate;
			m_pDecoderInfo = pDecoderInfo;
			m_nDecoderInfoSize = nDecoderInfoSize;

			return ESUCCESS;
		}

		virtual void terminate() {};

		virtual void dump(const char* strPref = "") const = 0;
};

/*
 * Audio decoder for single audio stream
 */
class CAudioSinkA : public CAudioSink
{
	protected:
		CThread						m_worker;		/* Worker thread */
		std::list<CAudioFrame*>		m_frames;		/* Audio frame list */
		mutable CCondition			m_cond;			/* List synchonisation/wakeup */
		atomic_t					m_bDone;		/* Cancellation flag */

		counter_t					m_nFrames;		/* DBG: Full processed frame count */

	public:
		CAudioSinkA();
		virtual ~CAudioSinkA();

	public:
		virtual void put(CAudioFrame* pFrame);

		virtual result_t init(unsigned int nSamplesPerFrame, unsigned int nClockRate,
							  const void* pDecoderInfo, size_t nDecoderInfoSize);
		virtual void terminate();

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual void wakeup();
		virtual void clear();

		virtual CAudioFrame* getNextFrame();
		virtual void processFrame(CAudioFrame* pFrame) = 0;

	private:
		void* worker(CThread* pThread, void* pData);
};

#endif /* __NET_MEDIA_AUDIO_SINK_H_INCLUDED__ */
