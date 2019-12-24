/*
 *	IP MultiCamera Test Application
 *	File Storage
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

#ifndef __STORAGE_H_INCLUDED__
#define __STORAGE_H_INCLUDED__

#include "shell/file.h"
#include "shell/counter.h"

#include "carbon/cstring.h"

#include "net_media/media_sink.h"

#define VIDEO_FPS					25

class CRtpFileWriter : public CVideoSinkV
{
	protected:
		CString		m_strFile;		/* Storage file name */
		CFile		m_file;			/* Storage file object */

		counter_t	m_nFrames;		/* DBG: written frames */
		counter_t	m_nErrors;		/* DBG: write errors */

	public:
		CRtpFileWriter(const char* strFilename);
		virtual ~CRtpFileWriter();

	public:
		virtual result_t init(int nFps, int nRate);
		virtual void terminate();

	protected:
		virtual void processNode(CRtpPlayoutNode* pNode);
};

#endif /* __STORAGE_H_INCLUDED__ */
