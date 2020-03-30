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
 *	Revision 1.0, 28.02.2017 12:07:17
 *		Initial revision.
 */

#include "carbon/carbon.h"

#include "net_media/subtitle/subtitle_sink.h"


/*******************************************************************************
 * CSubtitleSinkS class
 */

CSubtitleSinkS::CSubtitleSinkS()
{
}

CSubtitleSinkS::~CSubtitleSinkS()
{
}

/*
 * Initialise and start receiving an audio frames
 *
 * 		nClockRate			Subtitle clock rate
 * 		nWidth				Subtitle width
 * 		nHeight				Subtitle height
 *
 * Return: ESUCCESS, ...
 */
result_t CSubtitleSinkS::init(unsigned int nClockRate, uint16_t nWidth, uint16_t nHeight)
{
	shell_unused(nClockRate);
	shell_unused(nWidth);
	shell_unused(nHeight);

	counter_init(m_nFrames);

	return ESUCCESS;
}

/*
 * Deinitialise and terminate subtitle sink
 */
void CSubtitleSinkS::terminate()
{
}

/*******************************************************************************
 * Debugging support
 */

void CSubtitleSinkS::dump(const char* strPref) const
{
	log_dump("*** %sSubtitleSink: full frame count: %u\n", counter_get(m_nFrames));
}
