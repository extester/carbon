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
 *  Revision 1.0, 21.11.2016 15:20:33
 *      Initial revision.
 */

#ifndef __NET_MEDIA_AUDIO_FRAME_H_INCLUDED__
#define __NET_MEDIA_AUDIO_FRAME_H_INCLUDED__

#include "shell/shell.h"

#include "net_media/media_frame.h"

class CAudioServer;

class CAudioFrame : public CMediaFrame
{
	protected:
		CAudioServer*		m_pParent;

	public:
		CAudioFrame(CAudioServer* pParent);
	protected:
		virtual ~CAudioFrame();

	public:
		void put();
};

#define AUDIO_FRAME_NULL 		((CAudioFrame*)0)

#endif /* __NET_MEDIA_AUDIO_FRAME_H_INCLUDED__ */

