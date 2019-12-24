/*
 *	Carbon/Network MultiMedia Streaming Module
 *	Media frame base class
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 16.11.2016 18:44:52
 *	    Initial revision.
 */

#include "net_media/media_frame.h"

/*******************************************************************************
 * CMediaFrame class
 */

CMediaFrame::CMediaFrame(const char* strName) :
	CObject(strName),
	CRefObject()
{
}

CMediaFrame::~CMediaFrame()
{
}
