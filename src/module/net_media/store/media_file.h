/*
 *	Carbon/Network MultiMedia Streaming Module
 *	Multimedia file store base class
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 16.11.2016 18:46:49
 *	    Initial revision.
 */

#ifndef __MEDIA_FILE_H_INCLUDED__
#define __MEDIA_FILE_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/cstring.h"

#include "net_media/media_frame.h"

class CMediaFile
{
	protected:
		CString			m_strFilename;		/* Full filename */

	public:
		CMediaFile();
		virtual ~CMediaFile();

	public:
		const char* getFile() const { return m_strFilename; }
		virtual result_t create(const char* strFilename) = 0;
		virtual result_t close() = 0;

		virtual result_t insertVideoTrack(int nFps, uint32_t nRate = 0) = 0;
		virtual result_t insertAudioTrack(uint32_t nSamplesPerFrame, uint32_t nRate,
							const void* pDecoderInfo, size_t decoderInfoSize) = 0;

		virtual result_t writeVideoFrame(CMediaFrame* pFrame) = 0;
		virtual result_t writeAudioFrame(CMediaFrame* pFrame) = 0;

		virtual int getVideoFrameCount() const = 0;
		virtual int getAudioFrameCount() const = 0;

		virtual void dump(const char* strPref = "") const = 0;
		virtual void dumpDyn() const = 0;
};

#endif /* __MEDIA_FILE_H_INCLUDED__ */
