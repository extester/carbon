/*
 *	IP Camera Test Application
 *	MP4 File writer
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 24.10.2016 11:12:11
 *		Initial revision.
 */

#ifndef __MP4_WRITER_H_INCLUDED__
#define __MP4_WRITER_H_INCLUDED__

#include "shell/counter.h"

#include "carbon/carbon.h"

#include "net_media/media_sink.h"
#include "net_media/store/mp4_h264_file.h"

class CMp4Writer : public CVideoSinkV
{
	protected:
		CString			m_strFile;		/* Storage file name */
		CMp4H264File	m_file;			/* Storage MP4 file object */

		counter_t		m_nFrames;		/* DBG: written frames */
		counter_t		m_nErrors;		/* DBG: write errors */

	public:
		CMp4Writer(const char* strFilename);
		virtual ~CMp4Writer();

	public:
		virtual result_t init(int nFps, int nRate);
		virtual void terminate();

		virtual void dump(const char* strPref = "") const {
			m_file.dump(strPref);
		}

	protected:
		virtual void processNode(CRtpPlayoutNode* pNode);
};

#endif /* __MP4_WRITER_H_INCLUDED__ */
