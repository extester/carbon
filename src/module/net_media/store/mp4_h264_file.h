/*
 *	Carbon/Network MultiMedia Streaming Module
 *	Multimedia MP4 H264 file store
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 16.11.2016 18:51:17
 *	    Initial revision.
 */
/*
 * MP4 file:
 *
 * 	- contains up to 1 video H264 track and up to 1 audio track
 * 	- video track contant FPS
 * 	- audio track AAC format mono
 */

#ifndef __MP4_H264_FILE_H_INCLUDED__
#define __MP4_H264_FILE_H_INCLUDED__

#include <mp4v2/mp4v2.h>

#include "carbon/thread.h"

#include "net_media/store/mp4_h264/mp4av_h264.h"
#include "net_media/store/media_file.h"


#define MP4_SUBTITLE_LENGTH					256

#define MP4_TIME_RATE_GENERAL				90000

typedef struct nal_reader_t {
	const char* pData;
	int			size;
	int			pos;

	uint8_t*	buffer;
	uint32_t 	buffer_on;
	uint32_t 	buffer_size;
	uint32_t 	buffer_size_max;
} nal_reader_t;

typedef struct {
	struct {
		int size_min;
		int next;
		int cnt;
		int idx[17];
		int poc[17];
	} dpb;

	int cnt;
	int cnt_max;
	int *frame;
} h264_dpb_t;

class CMp4H264File : public CMediaFile
{
	protected:
		MP4FileHandle	m_hFile;			/* MP4 open file handle or 0 */
		mutable CMutex	m_lock;				/* I/O synchronisation */
		uint32_t		m_nTimeRate;		/* Default Time rate for tracks */

		/* Video Track parameters */
		MP4TrackId		m_videoTrackId;		/* Video track Id or MP4_INVALID_TRACK_ID */
		uint32_t		m_nVideoRate;		/* Video track Rate (90000) */
		int				m_nVideoFps;		/* Video track Frame per Seconds (FPS) */
		uint64_t		m_videoTimestamp;	/* Last written video frame timestamp */

		nal_reader_t 	nal;				/* H264 parameters */
		h264_decode_t 	h264_dec;
		h264_dpb_t 		h264_dpb;
		bool 			first;
		int32_t 		poc;
		bool 			nal_is_sync;
		bool 			slice_is_idr;
		uint8_t*		nal_buffer;
		uint32_t 		nal_buffer_size;
		uint32_t		nal_buffer_size_max;

		/* Audio Track parameters */
		MP4TrackId		m_audioTrackId;		/* Audio track Id or MP4_INVALID_TRACK_ID */
		uint32_t		m_nAudioRate;		/* Audio track data Rate */
		uint32_t		m_nSamplesPerFrame;
		uint64_t		m_audioTimestamp;	/* Last written audio frame timestamp */
		uint8_t*		m_pAudioBuf;		/* Temporary audio buffer for first sample */
		size_t			m_nAudioBufLen;		/* Temporary audio buffer data length */

		/* Subtitle Track parameters */
		MP4TrackId		m_subtitleTrackId;	/* Subtitle track Id or MP4_INVALID_TRACK_ID */
		uint32_t		m_nSubtitleRate;	/* Subtitle track data Rate */
		uint64_t		m_subtitleTimestamp;/* Last written subtitle frame timestamp */
		uint8_t			m_subtitleBuf[MP4_SUBTITLE_LENGTH];
		uint32_t		m_nSubtitleBufLen;

		/* Statistics */
		counter_t		m_nVideoFrames;		/* DBG: Video frames written */
		counter_t		m_nAudioFrames;		/* DBG: Audio frames written */
		counter_t		m_nSubtitleFrames;	/* DBG: Subtitle frames written */
		int 			m_nDump;			/* DBG: */

	public:
		CMp4H264File(uint32_t nTimeRate = MP4_TIME_RATE_GENERAL);
		virtual ~CMp4H264File();

	public:
		boolean_t isOpen() const { return m_hFile != 0; }

		virtual result_t create(const char* strFilename);
		virtual result_t close();

		virtual result_t insertVideoTrack(int nFps, uint32_t nRate = 0);
		virtual result_t insertAudioTrack(uint32_t nSamplesPerFrame, uint32_t nRate,
								const void* pDecoderInfo, size_t nDecoderInfoSize);
		virtual result_t insertSubtitleTrack(uint32_t nRate, uint16_t nWidth, uint16_t nHeight);

		virtual result_t writeVideoFrame(CMediaFrame* pFrame);
		virtual result_t writeAudioFrame(CMediaFrame* pFrame);
		virtual result_t writeSubtitleFrame(CMediaFrame* pFrame);

		virtual int getVideoFrameCount() const { return counter_get(m_nVideoFrames); }
		virtual int getAudioFrameCount() const { return counter_get(m_nAudioFrames); }
		virtual int getSubtitleFrameCount() const { return counter_get(m_nSubtitleFrames); }

		virtual void dump(const char* strPref = "") const;
		virtual void dumpDyn() const;

	private:
		void DpbInit(h264_dpb_t* p);
		void DpbClean(h264_dpb_t* p);
		void DpbUpdate(h264_dpb_t* p, int is_forced);
		void DpbFlush(h264_dpb_t* p);
		void DpbAdd(h264_dpb_t* p, int poc, int is_idr);
		int DpbFrameOffset(h264_dpb_t* p, int idx);
		bool remove_unused_sei_messages(nal_reader_t* nal, uint32_t header_size);
		bool RefreshReader(nal_reader_t* nal, uint32_t nal_start);
		bool LoadNal(nal_reader_t* nal);

		result_t doCreateVideoTrack(CMediaFrame* pFrame);
		result_t doWriteVideoFrame(CMediaFrame* pFrame);
		result_t doWriteVideoFinalise();

		result_t doWriteAudioFinalise();

		void dumpFrames() const;
};

#endif /* __MP4_H264_FILE_H_INCLUDED__ */
