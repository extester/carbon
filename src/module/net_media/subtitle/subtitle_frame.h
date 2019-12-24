/*
 *	Carbon/Network MultiMedia Streaming Module
 *  Class represents a single Subtitle Frame
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 28.02.2017 10:57:28
 *      Initial revision.
 */

#ifndef __NET_MEDIA_SUBTITLE_FRAME_H_INCLUDED__
#define __NET_MEDIA_SUBTITLE_FRAME_H_INCLUDED__

#include "shell/shell.h"

#include "net_media/media_frame.h"

#define NET_MEDIA_SUBTITLE_LENGTH		256

class CSubtitleServer;

class CSubtitleFrame : public CMediaFrame
{
	protected:
		CSubtitleServer*	m_pParent;			/* Subtitle server belong to */

		char 				m_buffer[NET_MEDIA_SUBTITLE_LENGTH];	/* Subtitle buffer */
		size_t				m_length;			/* Current subtitle length, bytes */

		uint64_t			m_timestamp;		/* Frame timestamp */
		hr_time_t			m_hrPts;			/* Frame presentation time */

	public:
		CSubtitleFrame(CSubtitleServer* pParent);
	protected:
		virtual ~CSubtitleFrame();

	public:
		virtual uint8_t* getData() { return (uint8_t*)m_buffer; }
		virtual size_t getSize() const { return m_length; }

		virtual uint64_t getTimestamp() const { return m_timestamp; }
		virtual void setTimestamp(uint64_t timestamp) { m_timestamp = timestamp; }

		virtual hr_time_t getPts() const { return m_hrPts; }

		void setData(uint64_t timestamp, hr_time_t hrPts, const char* strText);

		virtual void dump(const char* strPref = "") const;
};

#endif /* __NET_MEDIA_SUBTITLE_FRAME_H_INCLUDED__ */
