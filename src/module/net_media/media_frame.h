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
 *	Revision 1.0, 16.11.2016 18:42:44
 *	    Initial revision.
 */

#ifndef __MEDIA_FRAME_H_INCLUDED__
#define __MEDIA_FRAME_H_INCLUDED__

#include "shell/shell.h"
#include "shell/object.h"
#include "shell/ref_object.h"
#include "shell/hr_time.h"

class CMediaFrame : public CObject, public CRefObject
{
	public:
		explicit CMediaFrame(const char* strName);
	protected:
		virtual ~CMediaFrame();

	public:
		/*
		 * Frame data/size
		 */
		virtual uint8_t* getData() = 0;
		virtual size_t getSize() const = 0;

		/*
		 * Get/Set a media frame Decode Timestamp (DTS):
		 * 		- first timestamp is random value (starts from any value)
		 * 		- 64 bits unsigned value, no overflow
		 * 		- time rate value (i.e. 90000)
		 */
		virtual uint64_t getTimestamp() const = 0;
		virtual void setTimestamp(uint64_t timestamp) = 0;

		/*
		 * Get a media frame Presentation Timestamp (PTS)
		 */
		virtual hr_time_t getPts() const = 0;

		/*
		 * Debugging support
		 */
		virtual void dump(const char* strPref = "") const = 0;
};

#endif /* __MEDIA_FRAME_H_INCLUDED__ */
