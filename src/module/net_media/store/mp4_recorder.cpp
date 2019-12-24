/*
 *	Carbon/Network MultiMedia Streaming Module
 *	MP4 Audio/Video File Recorder
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 24.11.2016 13:44:47
 *	    Initial revision.
 */

#include "net_media/rtp_playout_buffer_h264.h"
#include "net_media/store/mp4_recorder.h"

/*******************************************************************************
 * CMp4Recorder class
 */

CMp4Recorder::CMp4Recorder(const char* strFile, int flags) :
	m_strFile(strFile),
	m_video(this),
	m_audio(this),
	m_subtitle(this),
	m_flags(flags),
	m_hrPts(HR_0),
	m_bLeadAudio(TRUE),
	m_nDropVideoLead(ZERO_COUNTER),
	m_nDropAudioLead(ZERO_COUNTER),
	m_tmVFirst(HR_0),
	m_tmVLast(HR_0),
	m_tmAFirst(HR_0),
	m_tmALast(HR_0)
{
}

CMp4Recorder::CMp4Recorder(int flags) :
	m_video(this),
	m_audio(this),
	m_subtitle(this),
	m_flags(flags),
	m_hrPts(HR_0),
	m_bLeadAudio(TRUE),
	m_nDropVideoLead(ZERO_COUNTER),
	m_nDropAudioLead(ZERO_COUNTER),
	m_tmVFirst(HR_0),
	m_tmVLast(HR_0),
	m_tmAFirst(HR_0),
	m_tmALast(HR_0)
{
}

CMp4Recorder::~CMp4Recorder()
{
}

int x_frames = 0;
//hr_time_t x_hrLast = 0;
hr_time_t hrFisrt = 0, hrLast = 0;
void CMp4Recorder::processVideoFrame(CMediaFrame* pFrame)
{
	CAutoLock				locker(m_lock);
	CRtpPlayoutNodeH264*	pNode;
	result_t				nresult;

	pNode = dynamic_cast<CRtpPlayoutNodeH264*>(pFrame);
	shell_assert(pNode);

	/*
	 * Drop leading non-key frames
	 */
	if ( m_hrPts != HR_0 || pNode->isIdrFrame() )  {
		//x_hrLast = hr_time_now();

//if ( m_hrPts == HR_0 && counter_get(m_nDropVideoLead) > 0 )  {
//	log_debug(L_GEN, "[mp4_rec] dropped %d lead video frames\n", counter_get(m_nDropVideoLead));
//}
		nresult = m_file.writeVideoFrame(pFrame);
		x_frames++;
		if ( nresult == ESUCCESS )  {
			if ( m_hrPts == HR_0 )  {
				m_hrPts = pFrame->getPts();
				m_tmVFirst = pFrame->getTimestamp();
				hrFisrt = hr_time_now();
			}
			else {
	/*uint64_t delta = pFrame->getTimestamp() - m_tmVLast;
	static uint64_t ddd = 0;
	if ( ddd != delta *//*!= 9000*//* ) {
		*//*log_debug(L_GEN, "---- frame TM: %llu (%d ms)---\n", delta, HR_TIME_TO_MILLISECONDS(hr_time_now()-hrLast));*//*
		log_debug(L_GEN, "---- frame TM: %llu => %llu (%d ms)---\n", ddd, delta, (uint32_t)HR_TIME_TO_MILLISECONDS(hr_time_now()-hrLast));
		ddd = delta;
	}*/
			}

			hrLast = hr_time_now();
			m_tmVLast = pFrame->getTimestamp();
		}
	}
	else {
		counter_inc(m_nDropVideoLead);
	}
}

void CMp4Recorder::processAudioFrame(CMediaFrame* pFrame)
{
	CAutoLock	locker(m_lock);
	boolean_t	hasVideo = (m_flags&MP4_RECORDER_HAS_VIDEO) != 0;

	/*
	 * Drop all audio frames until video frame
	 */
	if ( hasVideo && m_hrPts != HR_0 )  {
		/*
		 * Correct first audio frame timestamp
		 */
		if ( m_bLeadAudio ) {
#if 0
			uint64_t	tsFrame, tsDelta;
			hr_time_t	ptsFrame, ptsDelta;

 			ptsFrame = pFrame->getPts();
			if ( m_hrPts <= ptsFrame )  {
				ptsDelta = ptsFrame - m_hrPts;
				tsDelta = (uint64_t)(((hr_time_t)m_audio.getRate()*ptsDelta)/HR_TIME_RESOLUTION);

				tsFrame = pFrame->getTimestamp();
				tsFrame = tsFrame > tsDelta ? (tsFrame-tsDelta) : 0;
				pFrame->setTimestamp(tsFrame);
			}
#endif
			m_bLeadAudio = FALSE;
			m_tmAFirst = m_tmALast = pFrame->getTimestamp();
		}

		m_file.writeAudioFrame(pFrame);
		m_tmALast = pFrame->getTimestamp();
	}
	else {
		if ( !hasVideo ) {
			m_file.writeAudioFrame(pFrame);
		}
		else {
			counter_inc(m_nDropAudioLead);
		}
	}
}

void CMp4Recorder::processSubtitleFrame(CMediaFrame* pFrame)
{
	CAutoLock	locker(m_lock);
	boolean_t	hasVideo = (m_flags&MP4_RECORDER_HAS_VIDEO) != 0;

	if ( hasVideo && m_hrPts != HR_0 ) {
		m_file.writeSubtitleFrame(pFrame);
	}
}

/*
 * Insert video track to the MP4 file
 *
 * 		nFps		video frames per second
 * 		nRate		video clock rate (90000)
 *
 * Return: ESUCCESS, ...
 */
result_t CMp4Recorder::insertVideoTrack(int nFps, int nRate)
{
	CAutoLock	locker(m_lock);
	result_t	nresult;

	if ( !m_file.isOpen() )  {
		log_error(L_GEN, "[mp4_rec] %s: can't insert video track, file is not open\n",
				  m_strFile.cs());
		return EINVAL;
	}

	nresult = m_file.insertVideoTrack(nFps, (uint32_t)nRate);
	return nresult;
}

/*
 * Insert audio track to the MP4 file
 *
 * 		samplesPerFrame		samples per frame (1024 for faac AAC encoder)
 * 		nRate				data rate (i.e. 16000 for mono 16KHz (2 bytes per sample))
 * 		pDecoderInfo		specific decoder info data
 * 		nDecoderInfoSize	specific decoder info data size, bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CMp4Recorder::insertAudioTrack(unsigned int nSamplesPerFrame, unsigned int nRate,
						const void* pDecoderInfo, size_t nDecoderInfoSize)
{
	CAutoLock	locker(m_lock);
	result_t	nresult;

	if ( !m_file.isOpen() )  {
		log_error(L_GEN, "[mp4_rec] %s: can't insert audio track, file is not open\n",
				  m_strFile.cs());
		return EINVAL;
	}

	nresult = m_file.insertAudioTrack(nSamplesPerFrame, nRate, pDecoderInfo, nDecoderInfoSize);
	return nresult;
}

/*
 * Insert subtitle track to the MP4 file
 *
 * 		nClockRate			data rate
 * 		nWidth				subtitle width
 * 		nHeigth				subtitle height
 *
 * Return: ESUCCESS, ...
 */
result_t CMp4Recorder::insertSubtitleTrack(unsigned int nClockRate, uint16_t nWidth,
									  uint16_t nHeight)
{
	CAutoLock	locker(m_lock);
	result_t	nresult;

	if ( !m_file.isOpen() )  {
		log_error(L_GEN, "[mp4_rec] %s: can't insert audio track, file is not open\n",
				  m_strFile.cs());
		return EINVAL;
	}

	nresult = m_file.insertSubtitleTrack(nClockRate, nWidth, nHeight);
	return nresult;
}


/*
 * Initialise recorder and open output file
 *
 * Return: ESUCCESS, ...
 */
result_t CMp4Recorder::init()
{
	result_t	nresult = ENOENT;

	shell_assert(!m_strFile.isEmpty());
	shell_assert(!m_file.isOpen());

	if ( !m_strFile.isEmpty() ) {
		nresult = m_file.create(m_strFile);
	}

	return nresult;
}

void CMp4Recorder::terminate()
{
	CAutoLock	locker(m_lock);

	m_file.close();
}

/*******************************************************************************
 * Debugging support
 */

void CMp4Recorder::dump(const char* strPref) const
{
	log_dump("*** %sMp4Rec: file: %s, open: %s, flags: %d\n",
			 strPref, m_strFile.cs(), m_file.isOpen() ? "YES" : "NO",
			 m_flags);

	log_dump("    pts: %lld, drop lead V: %d, A: %d\n",
			 m_hrPts, counter_get(m_nDropVideoLead),
			 counter_get(m_nDropAudioLead));

	if ( m_flags&MP4_RECORDER_HAS_VIDEO )  {
		m_video.dump();
	}
	if ( m_flags&MP4_RECORDER_HAS_AUDIO )  {
		m_audio.dump();
	}
	if ( m_flags&MP4_RECORDER_HAS_SUBTITLE )  {
		m_subtitle.dump();
	}

	if ( m_file.isOpen() ) {
		m_file.dump(strPref);
	}
	else {
		log_dump("*** Mp4Rec: -NO MP4 File open-\n");
	}
}

void CMp4Recorder::dumpTmStat() const
{
	uint64_t 	tmLength = m_tmVLast-m_tmVFirst;
	hr_time_t	hrLength = tmLength*HR_TIME_RESOLUTION/m_video.getClockRate();
	uint32_t	msLength = (uint32_t)HR_TIME_TO_MILLISECONDS(hrLength);

	log_dump("REC VIDEO: Dropped lead frames: %u, Tm: first %llu, last: %llu, L: %llu, L: %u.%03u sec, frames: %d, time: %d ms\n",
			 counter_get(m_nDropVideoLead),
			 m_tmVFirst, m_tmVLast, tmLength,
			 msLength/1000, msLength%1000, x_frames,
			 (uint32_t)HR_TIME_TO_MILLISECONDS(hrLast-hrFisrt) );

	tmLength = m_tmALast-m_tmAFirst;
	if ( tmLength > 0 ) {
		hrLength = tmLength * HR_TIME_RESOLUTION / m_audio.getClockRate();
		msLength = (uint32_t)HR_TIME_TO_MILLISECONDS(hrLength);

		log_dump("REC AUDIO: Dropped lead frames: %u, Tm: %llu, Length: %u.%03u sec\n",
			counter_get(m_nDropAudioLead), tmLength, msLength / 1000, msLength % 1000);
	}
}
