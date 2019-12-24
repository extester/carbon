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
 *	Revision 1.0, 24.10.2016 11:22:27
 *		Initial revision.
 */

#include "net_media/rtp_playout_buffer_h264.h"

#include "mp4_writer.h"

#define CAMERA_FPS						25
#define CAMERA_CLOCK_RATE				RTP_VIDEO_CLOCK_RATE_DEFAULT

/*******************************************************************************
 * CMp4Writer
 */

CMp4Writer::CMp4Writer(const char* strFilename) :
	CVideoSinkV(),
	m_strFile(strFilename),
	m_file(CAMERA_CLOCK_RATE),
	m_nFrames(ZERO_COUNTER),
	m_nErrors(ZERO_COUNTER)
{
	shell_assert(strFilename && *strFilename != '\0');
}

CMp4Writer::~CMp4Writer()
{
	shell_assert(!m_file.isOpen());
}

uint64_t tmFirst = (uint64_t)-1, tmLast, tmPrev;
uint64_t tmFirstSkip = (uint64_t)-1;
int frameCount = 0;

void CMp4Writer::processNode(CRtpPlayoutNode* pNode)
{
	CMediaFrame*			pFrame;
	CRtpPlayoutNodeH264*	pNodeH264;
	result_t		nresult;

#if 0
static int count = 0;
static hr_time_t hr = 0;

/*static int n = 0;
static uint64_t lasttm = 0;

	if ( n < 200 )  {
		uint64_t	tm = pNode->getTimestamp();

		log_debug(L_GEN, "[%3d] frame tm: %llu, delta: %llu\n",
				  n, tm, tm-lasttm);

		n++;
		lasttm = tm;
	}*/

	if ( hr != HR_0 ) {
		hr_time_t now = hr_time_now(), delta = now-hr;

		count++;
		if ( delta >= HR_1SEC )  {
			//float fps = ((float)(count*1000))/(HR_TIME_TO_MILLISECONDS(delta));
			//log_debug(L_GEN, "FPS: %f\n", fps);

			count = 0;
			hr = hr_time_now();
		}

	}
	else {
		hr = hr_time_now();
		count = 1;
	}
#endif


	pFrame = dynamic_cast<CMediaFrame*>(pNode);
	shell_assert(pFrame);
	pNodeH264 = dynamic_cast<CRtpPlayoutNodeH264*>(pNode);
	shell_assert(pNodeH264);

	if ( tmFirst == (uint64_t)-1 && !pNodeH264->isIdrFrame() ) {
		uint64_t tm = tmFirstSkip == (uint64_t)-1 ?
					  	0 : (pFrame->getTimestamp()-tmFirst);
		log_debug(L_GEN, "SKIP LEADING NON-IDR FRAME TM %llu\n", tm);

		if ( tmFirstSkip == (uint64_t)-1 ) {
			tmFirstSkip = pFrame->getTimestamp();
		}

		return;
	}

	if ( tmFirst == (uint64_t)-1 )  {
		tmFirst = pNode->getTimestamp();
		tmPrev = pNode->getTimestamp();
	}
	else {
		uint64_t delta = pNode->getTimestamp() - tmPrev;
		if ( delta != 9000 )  {
			log_debug(L_GEN, "frame %3d: frame delta: %llu\n", frameCount, delta);
		}

		tmPrev = pNode->getTimestamp();

	}

	tmLast = pNode->getTimestamp();
	frameCount++;

	if ( m_file.isOpen() )  {
		nresult = m_file.writeVideoFrame(pFrame);
		if ( nresult == ESUCCESS ) {
			counter_inc(m_nFrames);
		}
		else {
			log_error(L_GEN, "[writer] WRITE FAILURE, result %d\n", nresult);
			counter_inc(m_nErrors);
		}
	}
}

result_t CMp4Writer::init(int nFps, int nRate)
{
	result_t	nresult;

	counter_init(m_nFrames);
	counter_init(m_nErrors);

	nresult = m_file.create(m_strFile);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[writer] failed to create output file %s, result %d\n",
				  m_strFile.cs(), nresult);
		return nresult;
	}

	nresult = m_file.insertVideoTrack(nFps, nRate);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[writer] %s: failed to insert video track, result %d\n",
				  m_strFile.cs(), nresult);
		m_file.close();
		return nresult;
	}

	log_info(L_GEN, "[writer] storage file created: %s\n", m_strFile.cs());

	nresult = CVideoSinkV::init(nFps, nRate);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[writer] failed to init decoder, result %d\n", nresult);
		m_file.close();
		return nresult;
	}

	return nresult;
}

void CMp4Writer::terminate()
{
	m_file.close();
	CVideoSinkV::terminate();

	log_info(L_GEN, "[writer] storage file closed: frames: %d, errors: %d\n",
			 	counter_get(m_nFrames), counter_get(m_nErrors));

	uint64_t 	tmLength = tmLast-tmFirst;
	hr_time_t	hrLength = tmLength*HR_TIME_RESOLUTION/90000;
	uint32_t	msLength = HR_TIME_TO_MILLISECONDS(hrLength);

	log_debug(L_GEN, "tmLength: %llu, length: %u.%u sec\n",
			  	tmLength, msLength/1000, msLength%1000);
}
