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
 *	Revision 1.0, 24.11.2016 13:25:07
 *	    Initial revision.
 */

#ifndef __NET_MEDIA_MP4_RECORDER_H_INCLUDED__
#define __NET_MEDIA_MP4_RECORDER_H_INCLUDED__

#include "shell/counter.h"

#include "carbon/carbon.h"

#include "net_media/audio/audio_sink.h"
#include "net_media/subtitle/subtitle_sink.h"
#include "net_media/media_sink.h"
#include "net_media/store/mp4_h264_file.h"

#define MP4_RECORDER_HAS_VIDEO			1
#define MP4_RECORDER_HAS_AUDIO			2
#define MP4_RECORDER_HAS_SUBTITLE		4

class CMp4Recorder
{
	class CVideoRecord : public CVideoSinkV
	{
		private:
			CMp4Recorder*		m_pParent;

		public:
			CVideoRecord(CMp4Recorder* pParent) :
				CVideoSinkV(),
				m_pParent(pParent)
			{
			}

			virtual ~CVideoRecord()
			{
			}

		public:
			virtual void processNode(CRtpPlayoutNode* pNode) {
				m_pParent->processVideoFrame(pNode);
			}

			virtual result_t init(int nFps, int nRate) {
				result_t	nresult;

				nresult = CVideoSinkV::init(nFps, nRate);
				if ( nresult == ESUCCESS )  {
					m_pParent->insertVideoTrack(nFps, nRate);
				}

				return nresult;
			}

			virtual void terminate() {
				CVideoSinkV::terminate();
			}
	};

	class CAudioRecord : public CAudioSinkA
	{
		private:
			CMp4Recorder*		m_pParent;

		public:
			CAudioRecord(CMp4Recorder* pParent) :
				CAudioSinkA(),
				m_pParent(pParent)
			{
			}

			virtual ~CAudioRecord()
			{
			}

		public:
			virtual void processFrame(CAudioFrame* pFrame) {
				m_pParent->processAudioFrame(pFrame);
			}

			virtual result_t init(unsigned int nSamplesPerFrame, unsigned int nRate,
								  const void* pDecoderInfo, size_t nDecoderInfoSize) {
				result_t	nresult;

				nresult = CAudioSinkA::init(nSamplesPerFrame, nRate,
											   pDecoderInfo, nDecoderInfoSize);
				if ( nresult == ESUCCESS )  {
					m_pParent->insertAudioTrack(nSamplesPerFrame, nRate,
											pDecoderInfo, nDecoderInfoSize);
				}

				return nresult;
			}

			virtual void terminate() {
				CAudioSinkA::terminate();
			}
	};

	class CSubtitleRecord : public CSubtitleSinkS
	{
		private:
			CMp4Recorder*		m_pParent;

		public:
			CSubtitleRecord(CMp4Recorder* pParent) :
				CSubtitleSinkS(),
				m_pParent(pParent)
			{
			}

			virtual ~CSubtitleRecord()
			{
			}

		public:
			virtual void processFrame(CSubtitleFrame* pFrame) {
				m_pParent->processSubtitleFrame(pFrame);
			}

			virtual result_t init(unsigned int nClockRate, uint16_t nWidth,
								  uint16_t nHeight) {
				result_t	nresult;

				nresult = CSubtitleSinkS::init(nClockRate, nWidth, nHeight);
				if ( nresult == ESUCCESS )  {
					m_pParent->insertSubtitleTrack(nClockRate, nWidth, nHeight);
				}

				return nresult;
			}

			virtual void terminate() {
				CSubtitleSinkS::terminate();
			}
	};

	protected:
		CString				m_strFile;				/* Full Mp4 filename */
		CMp4H264File		m_file;					/* MP4 file object */
		CVideoRecord		m_video;				/* Video source */
		CAudioRecord		m_audio;				/* Audio source */
		CSubtitleRecord		m_subtitle;				/* Subtitle source */

		int 				m_flags;				/* Recorder flags MP4_RECORDER_xxx */

		mutable CMutex		m_lock;					/* Multithread access synchronisation */
		hr_time_t			m_hrPts;				/* First video frame presentation time */

		boolean_t			m_bLeadAudio;			/* FALSE: no audio frames */

		counter_t			m_nDropVideoLead;		/* DBG: dropped leading video frames */
		counter_t			m_nDropAudioLead;		/* DBG: dropped leading audio frames */

		uint64_t 			m_tmVFirst, m_tmVLast;	/* DBG: Video timestamp statistics */
		uint64_t 			m_tmAFirst, m_tmALast;	/* DBG: Audio timestamp statistics */

	public:
		CMp4Recorder(const char* strFile, int flags);
		CMp4Recorder(int flags);
		virtual ~CMp4Recorder();

	public:
		void setFilename(const char* strFilename) { m_strFile = strFilename; }

		CVideoSink* getVideoSink() { return &m_video; }
		CAudioSink* getAudioSink() { return &m_audio; }
		CSubtitleSinkS* getSubtitleSink() { return &m_subtitle; }

		virtual void processVideoFrame(CMediaFrame* pFrame);
		virtual void processAudioFrame(CMediaFrame* pFrame);
		virtual void processSubtitleFrame(CMediaFrame* pFrame);

		virtual result_t insertVideoTrack(int nFps, int nRate);
		virtual result_t insertAudioTrack(unsigned int nSamplesPerFrame,
							unsigned int nRate, const void* pDecoderInfo,
							size_t nDecoderInfoSize);
		virtual result_t insertSubtitleTrack(unsigned int nClockRate, uint16_t nWidth,
											 uint16_t nHeight);

		void reset() {
			m_hrPts = HR_0;
			m_bLeadAudio = TRUE;
			m_nDropVideoLead = ZERO_COUNTER;
			m_nDropAudioLead = ZERO_COUNTER;
		}

		virtual result_t init();
		virtual void terminate();

		virtual void dump(const char* strPref = "") const;
		virtual void dumpTmStat() const;
};

#endif /* __NET_MEDIA_MP4_RECORDER_H_INCLUDED__ */
