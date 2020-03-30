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
 *  Revision 1.0, 28.02.2017 11:01:00
 *      Initial revision.
 */

#include <arpa/inet.h>

#include "net_media/subtitle/subtitle_server.h"
#include "net_media/subtitle/subtitle_frame.h"


/*******************************************************************************
 * CSubtitleFrame class
 */

CSubtitleFrame::CSubtitleFrame(CSubtitleServer* pParent) :
	CMediaFrame("stframe"),
	m_pParent(pParent)
{
	shell_assert(pParent != 0);
}

CSubtitleFrame::~CSubtitleFrame()
{
}

/*
 * Formatting subtitle text
 *
 * 		timestamp		frame timestamp
 * 		hrPts			frame presentation time
 * 		strText			subtitle text
 */
void CSubtitleFrame::setData(uint64_t timestamp, hr_time_t hrPts, const char* strText)
{
	uint16_t	length;

	struct tmp_data {
		uint16_t	length;
		uint8_t		data[NET_MEDIA_SUBTITLE_LENGTH-2];
	} __attribute__ ((packed)) *pData = (struct tmp_data*)m_buffer;

	if ( strText != 0 && *strText != '\0') {
		length = (uint16_t)strlen(strText);
		length = (uint16_t)sh_min(length, NET_MEDIA_SUBTITLE_LENGTH-2);

		pData->length = htons(length);
		UNALIGNED_MEMCPY(pData->data, strText, length);

		m_length = length+2;
	}
	else {
		m_length = 0;
	}

	m_timestamp = timestamp;
	m_hrPts = hrPts;
}

/*******************************************************************************
 * Debugging support
 */

void CSubtitleFrame::dump(const char* strPref) const
{
	log_dump("%sSubtitle frame (timestamp %u): '%s'\n", strPref,
			 m_timestamp, m_length ? m_buffer : "<empty>");
}
