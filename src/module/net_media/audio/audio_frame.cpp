/*
 *	Carbon/Network MultiMedia Streaming Module
 *  Class represents a single Audio Frame
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 21.11.2016 15:23:58
 *      Initial revision.
 */

#include "net_media/audio/audio_server.h"
#include "net_media/audio/audio_frame.h"

/*******************************************************************************
 * CAudioFrame class
 */

CAudioFrame::CAudioFrame(CAudioServer* pParent) :
	CMediaFrame("aframe"),
	m_pParent(pParent)
{
	shell_assert(pParent != 0);
}

CAudioFrame::~CAudioFrame()
{
}

void CAudioFrame::put()
{
	m_pParent->putFrame(this);
}
