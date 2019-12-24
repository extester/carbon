/*
 *	Carbon/Network MultiMedia Streaming Module
 *	Subtitle frame post processor
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 28.02.2017 12:02:23
 *		Initial revision.
 */

#ifndef __NET_MEDIA_SUBTITLE_SINK_H_INCLUDED__
#define __NET_MEDIA_SUBTITLE_SINK_H_INCLUDED__

#include "shell/counter.h"

#include "net_media/subtitle/subtitle_frame.h"

class CSubtitleSinkS
{
	protected:
		counter_t			m_nFrames;		/* DBG: Full processed frame count */

	public:
		CSubtitleSinkS();
		virtual ~CSubtitleSinkS();

	public:
		virtual result_t init(unsigned int nClockRate, uint16_t nWidth, uint16_t nHeight);
		virtual void terminate();

		virtual void processFrame(CSubtitleFrame* pFrame) = 0;

		virtual void dump(const char* strPref = "") const;
};

#endif /* __NET_MEDIA_SUBTITLE_SINK_H_INCLUDED__ */
