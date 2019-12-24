/*
 *	Carbon/Network MultiMedia Streaming Module
 *	Multimedia MP4 file store
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 16.11.2016 18:52:07
 *	    Initial revision.
 */

#include "carbon/memory.h"

#include "net_media/store/mp4_cache.h"
#include "net_media/store/mp4_h264_file.h"

#define TIMESTAMP_INVALID		((uint64_t)-1)

/*******************************************************************************
 * CMp4H264File class
 */

CMp4H264File::CMp4H264File(uint32_t nTimeRate) :
	CMediaFile(),
	m_hFile(0),
	m_nTimeRate(nTimeRate),

	m_videoTrackId(MP4_INVALID_TRACK_ID),
	m_nVideoRate(0),
	m_nVideoFps(0),
	m_videoTimestamp(TIMESTAMP_INVALID),
	first(true),
	poc(0),
	nal_is_sync(false),
	slice_is_idr(false),
	nal_buffer(NULL),
	nal_buffer_size(0),
	nal_buffer_size_max(0),

	m_audioTrackId(MP4_INVALID_TRACK_ID),
	m_nAudioRate(0),
	m_nSamplesPerFrame(0),
	m_audioTimestamp(TIMESTAMP_INVALID),
	m_pAudioBuf(NULL),
	m_nAudioBufLen(0),

	m_subtitleTrackId(MP4_INVALID_TRACK_ID),
	m_nSubtitleRate(0),
	m_subtitleTimestamp(TIMESTAMP_INVALID),
	m_nSubtitleBufLen(0),

	m_nVideoFrames(ZERO_COUNTER),
	m_nAudioFrames(ZERO_COUNTER),
	m_nSubtitleFrames(ZERO_COUNTER),
	m_nDump(0)
{
	_tbzero_object(nal);
	_tbzero_object(h264_dpb);
	_tbzero_object(h264_dec);
}

CMp4H264File::~CMp4H264File()
{
	close();
}

void CMp4H264File::DpbInit(h264_dpb_t *p)
{
	p->dpb.cnt = 0;
	p->dpb.next = 0;
	p->dpb.size_min = 0;

	p->cnt = 0;
	p->cnt_max = 0;
	p->frame = NULL;
}

void CMp4H264File::DpbClean(h264_dpb_t *p)
{
	memFree( p->frame );
	p->frame = 0;
}

void CMp4H264File::DpbUpdate(h264_dpb_t *p, int is_forced)
{
	int i;
	int pos;

	if (!is_forced && p->dpb.cnt < 16)
		return;

	/* find the lowest poc */
	pos = 0;
	for (i = 1; i < p->dpb.cnt; i++)
	{
		if (p->dpb.poc[i] < p->dpb.poc[pos])
			pos = i;
	}

	/* save the idx */
	if (p->dpb.idx[pos] >= p->cnt_max)
	{
		int inc = 1000 + (p->dpb.idx[pos]-p->cnt_max);
		p->cnt_max += inc;
		p->frame = (int*)memRealloc( p->frame, sizeof(int)*p->cnt_max );
		for (i=0;i<inc;i++)
			p->frame[p->cnt_max-inc+i] = -1; /* To detect errors latter */
	}
	p->frame[p->dpb.idx[pos]] = p->cnt++;

	/* Update the dpb minimal size */
	if (pos > p->dpb.size_min)
		p->dpb.size_min = pos;

	/* update dpb */
	for (i = pos; i < p->dpb.cnt-1; i++)
	{
		p->dpb.idx[i] = p->dpb.idx[i+1];
		p->dpb.poc[i] = p->dpb.poc[i+1];
	}
	p->dpb.cnt--;
}

void CMp4H264File::DpbFlush(h264_dpb_t *p)
{
	while (p->dpb.cnt > 0)
		DpbUpdate( p, true );
}

void CMp4H264File::DpbAdd(h264_dpb_t *p, int poc, int is_idr)
{
	if (is_idr)
		DpbFlush( p );

	p->dpb.idx[p->dpb.cnt] = p->dpb.next;
	p->dpb.poc[p->dpb.cnt] = poc;
	p->dpb.cnt++;
	p->dpb.next++;

	DpbUpdate( p, false );
}

int CMp4H264File::DpbFrameOffset(h264_dpb_t *p, int idx)
{
	if (idx >= p->cnt)
		return 0;
	if (p->frame[idx] < 0)
		return p->dpb.size_min; /* We have an error (probably broken/truncated bitstream) */

	return p->dpb.size_min + p->frame[idx] - idx;
}

bool CMp4H264File::remove_unused_sei_messages(nal_reader_t *nal, uint32_t header_size)
{
	uint32_t buffer_on = header_size;
	buffer_on++; // increment past SEI message header

	while (buffer_on < nal->buffer_on) {
		uint32_t payload_type, payload_size, start, size;
		if (nal->buffer[buffer_on] == 0x80) {
			// rbsp_trailing_bits
			return true;
		}

		if (nal->buffer_on - buffer_on <= 2) {
#if 0
			memset(nal->buffer + buffer_on, 0,
					nal->buffer_on - buffer_on);
			nal->buffer_on = buffer_on;
#endif
			return true;
		}

		start = buffer_on;
		payload_type = h264_read_sei_value(nal->buffer + buffer_on, &size);
#ifdef DEBUG_H264
		printf("sei type %d size %d on %d\n", payload_type,
				size, buffer_on);
#endif
		buffer_on += size;
		payload_size = h264_read_sei_value(nal->buffer + buffer_on, &size);
		buffer_on += size + payload_size;
#ifdef DEBUG_H264
		printf("sei size %d size %d on %d nal %d\n",
				payload_size, size, buffer_on, nal->buffer_on);
#endif
		if (buffer_on > nal->buffer_on) {
			//log_debug(L_MP4FILE, "[mp4_file] Error decoding sei message\n");
			return false;
		}

		switch (payload_type) {
			case 3:
			case 10:
			case 11:
			case 12:
				_tmemmove(nal->buffer + start,
						nal->buffer + buffer_on,
						nal->buffer_size - buffer_on);
				nal->buffer_size -= buffer_on - start;
				nal->buffer_on -= buffer_on - start;
				buffer_on = start;
				break;
		}
	}

	if (nal->buffer_on == header_size) return false;
	return true;
}

bool CMp4H264File::RefreshReader(nal_reader_t *nal, uint32_t nal_start)
{
	uint32_t bytes_left;
	uint32_t bytes_read;
#ifdef DEBUG_H264_READER
	printf("refresh - start %u buffer on %u size %u\n",
			nal_start, nal->buffer_on, nal->buffer_size);
#endif
	if (nal_start != 0) {
		if (nal_start > nal->buffer_size) {
#ifdef DEBUG_H264
			printf("nal start is greater than buffer size\n");
#endif
			nal->buffer_on = 0;
		}
		else {
			bytes_left = nal->buffer_size - nal_start;
			if (bytes_left > 0) {
				_tmemmove(nal->buffer,
						nal->buffer + nal_start,
						bytes_left);
				nal->buffer_on -= nal_start;
#ifdef DEBUG_H264_READER
				printf("move %u new on is %u\n", bytes_left, nal->buffer_on);
#endif
			}
			else {
				nal->buffer_on = 0;
			}
			nal->buffer_size = bytes_left;
		}
	}
	else {
		if (nal->pos >= nal->size /*feof(nal->ifile)*/) {
			return false;
		}
		nal->buffer_size_max += 4096 * 4;
		uint8_t* tmp = (uint8_t *)memRealloc(nal->buffer, nal->buffer_size_max);
		if ( tmp == NULL )  {
			return false;
		}
		nal->buffer = tmp;
	}

	bytes_read = MIN(nal->size-nal->pos, (int)(nal->buffer_size_max - nal->buffer_size));
	UNALIGNED_MEMCPY(nal->buffer + nal->buffer_size, nal->pData+nal->pos, bytes_read);
	nal->pos += bytes_read;
	/*bytes_read = fread(nal->buffer + nal->buffer_size,
		     1,
		     nal->buffer_size_max - nal->buffer_size,
		     nal->ifile);*/

	if (bytes_read == 0) return false;
#ifdef DEBUG_H264_READER
	printf("read %u of %u\n", bytes_read,
			nal->buffer_size_max - nal->buffer_size);
#endif
	nal->buffer_size += bytes_read;
	return true;
}

bool CMp4H264File::LoadNal(nal_reader_t *nal)
{
	if (nal->buffer_on != 0 || nal->buffer_size == 0) {
		if (RefreshReader(nal, nal->buffer_on) == false) {
#ifdef DEBUG_H264
			printf("refresh returned 0 - buffer on is %u, size %u\n",
					nal->buffer_on, nal->buffer_size);
#endif
			if (nal->buffer_on >= nal->buffer_size) return false;
			// continue
		}
	}

	// find start code
	uint32_t start;
	if (h264_is_start_code(nal->buffer) == false) {
		start = h264_find_next_start_code(nal->buffer, nal->buffer_size);
		RefreshReader(nal, start);
	}

	while ((start = h264_find_next_start_code(nal->buffer + 4, nal->buffer_size - 4)) == 0) {
		if (RefreshReader(nal, 0) == false) {
			// end of file - use the last NAL
			nal->buffer_on = nal->buffer_size;
			return true;
		}
	}

	nal->buffer_on = start + 4;
	return true;
}

/*
 * Create a new MP4 file (existing files are truncated)
 *
 * 		strFilename			full filename
 *
 * Return: ESUCCESS, ...
 */
result_t CMp4H264File::create(const char* strFilename)
{
	CAutoLock	locker(m_lock);
	const MP4FileProvider*	pFileProvider;

	log_debug(L_MP4FILE_FL, "[mp4_file] creating file: %s\n", strFilename);

	shell_assert(m_hFile == 0);
	shell_assert(m_videoTrackId == MP4_INVALID_TRACK_ID);
	shell_assert(m_nVideoFps == 0);
	shell_assert(m_nVideoRate == 0);
	shell_assert(m_audioTrackId == MP4_INVALID_TRACK_ID);
	shell_assert(m_subtitleTrackId == MP4_INVALID_TRACK_ID);
	shell_assert(m_nSubtitleRate == 0);

	pFileProvider = CMp4Cache::getCache()->getFileProvider();

	m_hFile = MP4CreateWithProvider(strFilename, pFileProvider, 0/*createFlags*/);
	if ( !m_hFile )  {
		return EIO;
	}

	m_strFilename = strFilename;

	MP4SetTimeScale(m_hFile, m_nTimeRate);

	m_videoTrackId = MP4_INVALID_TRACK_ID;
	m_audioTrackId = MP4_INVALID_TRACK_ID;
	m_subtitleTrackId = MP4_INVALID_TRACK_ID;

	counter_init(m_nVideoFrames);
	counter_init(m_nAudioFrames);
	counter_init(m_nSubtitleFrames);
	m_nDump = 0;

	return ESUCCESS;
}

result_t CMp4H264File::close()
{
	CAutoLock	locker(m_lock);
	result_t	nresult = ESUCCESS, nr;

	if ( isOpen() ) {
		log_debug(L_MP4FILE_FL, "[mp4_file] close file: %s\n", getFile());

		if ( m_videoTrackId != MP4_INVALID_TRACK_ID )  {
			nresult = doWriteVideoFinalise();
		}

		if ( m_videoTrackId != MP4_INVALID_TRACK_ID )  {
			nr = doWriteAudioFinalise();
			nresult_join(nresult, nr);
		}

		MP4Close(m_hFile, 0);
		m_hFile = 0;
	}

	SAFE_FREE(nal.buffer);
	SAFE_FREE(nal_buffer);
	DpbClean(&h264_dpb);

	SAFE_FREE(m_pAudioBuf);
	m_nAudioBufLen = 0;

	m_videoTrackId = MP4_INVALID_TRACK_ID;
	m_audioTrackId = MP4_INVALID_TRACK_ID;
	m_nVideoFps = 0;
	m_nVideoRate = 0;
	m_nAudioRate = 0;
	m_nSamplesPerFrame = 0;

	m_subtitleTrackId = MP4_INVALID_TRACK_ID;
	m_nSubtitleBufLen = 0;
	m_nSubtitleRate = 0;

	return nresult;
}

/*
 * Insert a Video track to the MP4 file
 *
 * 		nFps		Frames per second
 * 		nRate		time rate (i.e. 90000) (may be 0)
 *
 * Return: ESUCCESS, EINVAL
 */
result_t CMp4H264File::insertVideoTrack(int nFps, uint32_t nRate)
{
	CAutoLock	locker(m_lock);

	//log_debug(L_MP4FILE_FL, "[mp4_file] %s: inserting VIDEO track\n", getFile());

	if ( !isOpen() )  {
		log_error(L_MP4FILE, "[mp4_file] can't insert video track: file is not open\n");
		return EFAULT;
	}

	if ( m_nVideoFps != 0 )  {
		/* TODO: */
		if ( m_nVideoFps != nFps ) {
			log_error(L_MP4FILE, "[mp4_file] %s: can't insert video track: duplicated\n",
					  getFile());
			return EINVAL;
		}
		return ESUCCESS;
	}

	if ( nFps == 0 || nRate == 0 )  {
		log_error(L_MP4FILE, "[mp4_file] %s: can't insert video track: invalid params: "
					"FPS %d, rate %d\n", getFile(), nFps, nRate);
		return EINVAL;
	}

	m_nVideoFps = nFps;
	m_nVideoRate = nRate != 0  ? nRate : m_nTimeRate;

	return ESUCCESS;
}

/*
 * Create a new empty video track based on frame
 *
 * 		pFrame		first track frame to parse track parameters
 *
 * Return: ESUCCESS, EINVAL
 */
result_t CMp4H264File::doCreateVideoTrack(CMediaFrame* pFrame)
{
	bool		have_seq = false;
	uint8_t 	nal_type;
	uint8_t 	AVCProfileIndication = 0;
	uint8_t 	profile_compat = 0;
	uint8_t 	AVCLevelIndication = 0;
	uint32_t 	mp4FrameDuration;

	const char*	pData = (const char*)pFrame->getData();
	int 		size = (int)pFrame->getSize();

	shell_assert(isOpen());
	shell_assert(m_videoTrackId == MP4_INVALID_TRACK_ID);
	shell_assert(m_nVideoFps != 0);
	shell_assert(m_nVideoRate != 0);

	if ( !pData || size == 0 )  {
		log_debug(L_MP4FILE, "[mp4_file] %s: can't create video track, invalid frame data\n",
				  getFile());
		return EINVAL;
	}

	_tbzero_object(nal);
	nal.pData = pData;
	nal.size = size;

	/*
	 * Find sequence header
	 */

	while (have_seq == false && LoadNal(&nal) ) {
		nal_type = h264_nal_unit_type(nal.buffer);
		if (nal_type == H264_NAL_TYPE_SEQ_PARAM) {
			have_seq = true;
			uint32_t offset;

			if (nal.buffer[2] == 1) offset = 3;
			else offset = 4;

			// skip the nal type byte
			AVCProfileIndication = nal.buffer[offset + 1];
			profile_compat = nal.buffer[offset + 2];
			AVCLevelIndication = nal.buffer[offset + 3];
			if (h264_read_seq_info(nal.buffer, nal.buffer_on, &h264_dec) == -1) {
				log_debug(L_MP4FILE, "[mp4_file] %s: Could not decode Sequence header\n", getFile());
				memFree(nal.buffer);
				nal.buffer = NULL;
				return EINVAL;
			}
		}
	}

	nal.pos = 0;
	nal.buffer_size = 0;
	nal.buffer_on = 0;
	nal.buffer_size_max = 0;
	SAFE_FREE(nal.buffer);

	if ( !have_seq )  {
		/* Skip frame */
		log_debug(L_MP4FILE, "[mp4_file] %s: no sequence header, skip starting frame\n", getFile());
		return EINVAL;
	}

	mp4FrameDuration = m_nVideoRate/m_nVideoFps;

	/*
	 * Create the new video track
	 */
	m_videoTrackId = MP4AddH264VideoTrack(
		m_hFile,
		m_nVideoRate,
		mp4FrameDuration,
		h264_dec.pic_width,
		h264_dec.pic_height,
		AVCProfileIndication,
		profile_compat,
		AVCLevelIndication,
		3);

	if ( m_videoTrackId == MP4_INVALID_TRACK_ID ) {
		log_debug(L_MP4FILE, "[mp4_file] %s: can't create video track\n", getFile());
		return EINVAL;
	}

	MP4SetVideoProfileLevel(m_hFile, 0x7f);

	/*
	 * Prepare to write frames
	 */
	m_videoTimestamp = TIMESTAMP_INVALID;
	first = true;
	poc = 0;
	nal_is_sync = false;
	slice_is_idr = false;
	nal_buffer = NULL;
	nal_buffer_size = 0;
	nal_buffer_size_max = 0;

	_tbzero_object(h264_dec);
	DpbInit(&h264_dpb);

	return ESUCCESS;
}

/*void dump_long(const uint8_t* pData, size_t size)
{
	size_t 	blocks = size/32, rest = size%32, i;
	const uint8_t* p = pData;

	for(i=0; i<blocks; i++)  {
		log_dump_bin(p, 32, "");
		p += 32;
	}

	if ( rest > 0 )  {
		log_dump_bin(p, rest, "");
	}
}*/

result_t CMp4H264File::doWriteVideoFrame(CMediaFrame* pFrame)
{
	MP4Duration 	mp4Duration;
	bool 			bResult;

	const char*		pData = (const char*)pFrame->getData();
	int 			size = (int)pFrame->getSize();

	shell_assert(isOpen());
	shell_assert(m_videoTrackId != MP4_INVALID_TRACK_ID);
	shell_assert(m_nVideoFps != 0);
	shell_assert(m_nVideoRate != 0);

	if ( !pData || size == 0 )  {
		return ESUCCESS;
	}

	nal.pData = pData;
	nal.size = size;
	nal.pos = 0;

	while ( LoadNal(&nal) != false ) {
		uint32_t header_size;
		header_size = nal.buffer[2] == 1 ? 3 : 4;
		bool boundary = (bool)h264_detect_boundary(nal.buffer,
											 nal.buffer_on,
											 &h264_dec);
#ifdef DEBUG_H264
		log_debug(L_MP4, "*** Nal size %u, boundary %u\n", nal.buffer_on, boundary);
#endif

		if (boundary && first == false) {
			// write the previous sample
			if (nal_buffer_size != 0) {
#ifdef DEBUG_H264
				log_debug(L_MP4, "*** sid %d writing %u\n", counter_get(m_nVideoSamples),
								nal_buffer_size);
#endif
				shell_assert(m_videoTimestamp != TIMESTAMP_INVALID);

				mp4Duration = m_videoTimestamp != TIMESTAMP_INVALID ?
							  (pFrame->getTimestamp()-m_videoTimestamp) :
							  (m_nVideoRate/m_nVideoFps);

				bResult = MP4WriteSample(
					m_hFile,
					m_videoTrackId,
					nal_buffer,
					nal_buffer_size,
					mp4Duration,
					0,
					nal_is_sync);

				if ( !bResult ) {
					log_debug(L_MP4FILE, "[mp4_file] %s: can't write video frame %u\n",
							  getFile(), counter_get(m_nVideoFrames));
					return EIO;
				}

				counter_inc(m_nVideoFrames);
				//m_videoTimestamp = pFrame->getTimestamp();

				DpbAdd( &h264_dpb, poc, slice_is_idr );
				nal_is_sync = false;
#ifdef DEBUG_H264
				printf("wrote frame %d " U64 "\n", nal_buffer_size, thisTime);
#endif
				nal_buffer_size = 0;
			}
		}

		bool copy_nal_to_buffer = false;

		if (h264_nal_unit_type_is_slice(h264_dec.nal_unit_type)) {
			// copy all seis, etc before indicating first
			first = false;
			copy_nal_to_buffer = true;
			slice_is_idr = h264_dec.nal_unit_type == H264_NAL_TYPE_IDR_SLICE;
			poc = h264_dec.pic_order_cnt;

			nal_is_sync = h264_slice_is_idr(&h264_dec);
			m_videoTimestamp = pFrame->getTimestamp();
		}
		else {
			switch (h264_dec.nal_unit_type) {
				case H264_NAL_TYPE_SEQ_PARAM:
					// doesn't get added to sample buffer
					// remove header
					MP4AddH264SequenceParameterSet(m_hFile, m_videoTrackId,
												   nal.buffer + header_size,
												   nal.buffer_on - header_size);

					if ( (nal.buffer_on - header_size) > 30 ) {
						log_error(L_GEN, "-----------!!!!!!!!!!!!!!!!!!!!!!!!!111-----------\n");
						log_error(L_GEN, "----- SEQ length %d\n",(nal.buffer_on - header_size));
					}

					break;

				case H264_NAL_TYPE_PIC_PARAM:
					// doesn't get added to sample buffer
					MP4AddH264PictureParameterSet(m_hFile, m_videoTrackId,
												  nal.buffer + header_size,
												  nal.buffer_on - header_size);

					{
						/*static uint8_t pps[5000];
						static int len = 0;
						int l = (nal.buffer_on - header_size);
						uint8_t* p = nal.buffer + header_size;

						if ( l == 0 || l != len || memcmp(pps, p, l) != 0 ) {
							log_debug(L_GEN, "[mp4_file(%s)] ++++ PPS: %d bytes\n", getFile(), l);
							log_dump_bin(p, l > 30 ? 30 : l, "[mp4_file(%s)] PPS: ", getFile());

							if ( l > 5000 ) { l = 5000; }
							memcpy(pps, p, l);
							len = l;
						}*/

if ( (nal.buffer_on - header_size) > 30 )  {
	//int l = (nal.buffer_on - header_size);
	//uint8_t* p = nal.buffer + header_size;

	log_error(L_GEN, "-----------!!!!!!!!!!!!!!!!!!!!!!!!!0000-----------\n");
	log_error(L_GEN, "----- PPS length %d\n", (nal.buffer_on - header_size));
	/*log_error(L_GEN, "boundary=%d, first=%d, pData=%lXh, pDataSize %u, nal.size=%u, nal.pos=%u, nal.buffer %lXh\n",
			  		boundary, first, pData, size, nal.size, nal.pos, nal.buffer);
	log_error(L_GEN, "nal.buffer_size=%u, nal.buffer_on(nal-size): %u, header_size %u, pMediaFrame %Xh\n",
			  nal.buffer_size, nal.buffer_on, header_size, pFrame);
	log_dump_bin(p, l > 30 ? 30 : l, "[mp4_file(%s)] PPS DATA(%Xh): ", getFile(), p);
	log_dump_bin(nal.buffer, 64, "[mp4_file(%s)] nal.buffer: ", getFile());*/

	//xdump_nal((const uint8_t*)pData, size);
	//dump_long((const uint8_t*)pData, MIN(size, 256));
}
					}
					break;

				case H264_NAL_TYPE_FILLER_DATA:
					// doesn't get copied
					break;

				case H264_NAL_TYPE_SEI:
					copy_nal_to_buffer = remove_unused_sei_messages(&nal, header_size);
					break;

				case H264_NAL_TYPE_ACCESS_UNIT:
					// note - may not want to copy this - not needed
				default:
					copy_nal_to_buffer = true;
					break;
			}
		}

		if (copy_nal_to_buffer) {
			uint32_t to_write;

			bResult = true;
			to_write = nal.buffer_on - header_size;
			if (to_write + 4 + nal_buffer_size > nal_buffer_size_max) {
				nal_buffer_size_max += nal.buffer_on + 4;

				uint8_t* tmp = (uint8_t *)memRealloc(nal_buffer, nal_buffer_size_max);
				if ( tmp )  {
					nal_buffer = tmp;
				}
				else {
					nal_buffer_size_max -= nal.buffer_on + 4;
					bResult = false;
				}
			}

			if ( bResult )  {
				nal_buffer[nal_buffer_size] = (to_write >> 24) & 0xff;
				nal_buffer[nal_buffer_size + 1] = (to_write >> 16) & 0xff;
				nal_buffer[nal_buffer_size + 2] = (to_write >> 8)& 0xff;
				nal_buffer[nal_buffer_size + 3] = to_write & 0xff;
				UNALIGNED_MEMCPY(nal_buffer + nal_buffer_size + 4,
					   nal.buffer + header_size,
					   to_write);
#ifdef DEBUG_H264
				printf("copy nal - to_write %u offset %u total %u\n",
    				  to_write, nal_buffer_size, nal_buffer_size + 4 + to_write);
    		  	printf("header size %d bytes after %02x %02x\n",
    				  header_size, nal.buffer[header_size],
    				  nal.buffer[header_size + 1]);
#endif
				nal_buffer_size += to_write + 4;
			}
		}
	}

	return ESUCCESS;
}

/*
 * Write a video frame to the file
 *
 * 		pFrame		media frame containing M
 */
result_t CMp4H264File::writeVideoFrame(CMediaFrame* pFrame)
{
	CAutoLock	locker(m_lock);
	result_t	nresult;

	shell_assert(pFrame);

	if ( !isOpen() )  {
		return EFAULT;			/* File is not open */
	}

	if ( m_nVideoFps == 0 )  {
		return EFAULT;			/* No video track inserted */
	}

	if ( m_videoTrackId == MP4_INVALID_TRACK_ID )  {
		/* Create video track */
		nresult = doCreateVideoTrack(pFrame);
		if ( nresult != ESUCCESS )  {
			return nresult;
		}
	}

	nresult = doWriteVideoFrame(pFrame);

	if ( logger_is_enabled(LT_DEBUG|L_MP4FILE_DYN) ) {
		m_nDump++;
		if ( (m_nDump%(m_nVideoFps*5)) == 0 ) {
			dumpFrames();
		}
	}

	return nresult;
}

result_t CMp4H264File::doWriteVideoFinalise()
{
	shell_assert(isOpen());
	shell_assert(m_videoTrackId != MP4_INVALID_TRACK_ID);
	shell_assert(m_nVideoFps != 0);
	shell_assert(m_nVideoRate != 0);

	//log_debug(L_GEN, "MP4FILE: tm video = %llu, tm audio = %llu\n", tmVLength, tmALength);

#if 0
	if ( nal_buffer_size != 0 ) {
		MP4Duration		mp4Duration;
		bool			bResult;

		//samplesWritten++;
		//double thiscalc;
		//thiscalc = samplesWritten;
		//thiscalc *= Mp4TimeScale;
		//thiscalc /= VideoFrameRate;

		//thisTime = (MP4Duration)thiscalc;
		//MP4Duration dur;
		//dur = thisTime - lastTime;

		mp4Duration = m_nVideoRate/m_nVideoFps; /* TODO */

		bResult = MP4WriteSample(
			m_hFile,
			m_videoTrackId,
			nal_buffer,
			nal_buffer_size,
			mp4Duration,
			0,
			nal_is_sync);

		if ( bResult ) {
			DpbAdd(&h264_dpb, h264_dec.pic_order_cnt, slice_is_idr);
			counter_inc(m_nVideoFrames);
		}
		else {
			log_debug(L_MP4FILE, "[mp4_file] %s: can't write final video frame %u\n",
					  getFile(), counter_get(m_nVideoFrames));
		}
	}
#endif
	DpbFlush(&h264_dpb);

	if (h264_dpb.dpb.size_min > 0) {
		unsigned int ix;
		uint32_t mp4FrameDuration = m_nVideoRate/m_nVideoFps;

		for (ix = 0; ix < (uint32_t)counter_get(m_nVideoFrames); ix++) {
			const int offset = DpbFrameOffset(&h264_dpb, ix);
			MP4SetSampleRenderingOffset(m_hFile, m_videoTrackId, 1 + ix,
										offset * mp4FrameDuration);
		}
	}

	DpbClean(&h264_dpb);
	SAFE_FREE(nal_buffer);

	return ESUCCESS;
}

#define AUDIO_BUF_SIZE(spf) 	((spf)*4)

/*
 * Insert an Audio track to the MP4 file
 *
 * 		samplesPerFrame		samples per frame (1024 for faac AAC encoder)
 * 		nRate				data rate (i.e. 16000 for mono 8KHz (2 bytes per sample))
 * 		pDecoderInfo		specific decoder info data
 * 		nDecoderInfoSize		specific decoder info data size, bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CMp4H264File::insertAudioTrack(uint32_t nSamplesPerFrame, uint32_t nRate,
								  const void* pDecoderInfo, size_t nDecoderInfoSize)
{
	CAutoLock	locker(m_lock);
	result_t	nresult = ESUCCESS;

	//log_debug(L_MP4FILE_FL, "[mp4_file] %s: inserting AUDIO track\n", getFile());

	if ( !isOpen() )  {
		log_error(L_MP4FILE, "[mp4_file] can't insert audio track, file is not open\n");
		return EFAULT;
	}

	if ( m_pAudioBuf != NULL )  {
		log_error(L_MP4FILE, "[mp4_file] %s: can't insert audio track: "
					"duplicated Audio track\n", getFile());
		return EINVAL;
	}

	shell_assert(m_nAudioRate == 0);
	shell_assert(m_nAudioBufLen == 0);

	if ( nSamplesPerFrame == 0 || nRate == 0 )  {
		log_error(L_MP4FILE, "[mp4_file] %s: can't insert audio track: "
					"invalid audio parameters: SPF %d, rate %d\n",
				  	getFile(), nSamplesPerFrame, nRate);
		return EINVAL;
	}

	m_pAudioBuf = (uint8_t*)memAlloc(AUDIO_BUF_SIZE(nSamplesPerFrame)); /* TODO: *SampleSize? */
	if ( m_pAudioBuf == NULL )  {
		log_error(L_MP4FILE, "[mp4_file] %s: can't insert audio track: out of memory\n", getFile());
		return ENOMEM;
	}
	m_nAudioBufLen = 0;

	m_audioTrackId = MP4AddAudioTrack(m_hFile, nRate, nSamplesPerFrame,
									  MP4_MPEG4_AUDIO_TYPE);

	if ( m_audioTrackId != MP4_INVALID_TRACK_ID ) {
		MP4SetAudioProfileLevel(m_hFile, 0x0F);
		MP4SetTrackIntegerProperty(m_hFile, m_audioTrackId,
				"mdia.minf.stbl.stsd.mp4a.channels", 1);

		if ( pDecoderInfo && nDecoderInfoSize > 0 ) {
			MP4SetTrackESConfiguration(m_hFile, m_audioTrackId,
						(const uint8_t*)pDecoderInfo, nDecoderInfoSize);
		}

		m_nAudioRate = nRate;
		m_nSamplesPerFrame = nSamplesPerFrame;
		m_audioTimestamp = TIMESTAMP_INVALID;
		counter_init(m_nAudioFrames);
	}
	else {
		log_error(L_MP4FILE, "[mp4_file] %s: can't create audio track\n", getFile());
		SAFE_FREE(m_pAudioBuf);
		nresult = EIO;
	}

	return nresult;
}

result_t CMp4H264File::writeAudioFrame(CMediaFrame* pFrame)
{
	CAutoLock		locker(m_lock);
	const uint8_t*	pData;
	int 			size;
	MP4Duration		mp4Duration;
	bool			bResult;
	result_t		nresult = ESUCCESS;

	shell_assert(pFrame);
	shell_assert(m_pAudioBuf != NULL);

	if ( !isOpen() )  {
		return EFAULT;			/* File is not open */
	}

	if ( m_audioTrackId == MP4_INVALID_TRACK_ID )  {
		return EFAULT;			/* No audio track inserted/created */
	}

	pData = (const uint8_t*)pFrame->getData();
	size = (int)pFrame->getSize();

	if ( !pData || size == 0 )  {
		return ESUCCESS;
	}

	if ( m_nAudioBufLen > 0 )  {
		/*
		 * Write previous audio frame
		 */
		shell_assert(m_audioTimestamp != TIMESTAMP_INVALID);

		mp4Duration = pFrame->getTimestamp()-m_audioTimestamp;

		bResult = MP4WriteSample(m_hFile, m_audioTrackId, m_pAudioBuf,
								 m_nAudioBufLen, mp4Duration);
		if ( bResult ) {
			counter_inc(m_nAudioFrames);
		}
		else {
			nresult = EIO;
		}

		m_nAudioBufLen = 0;
	}

	m_nAudioBufLen = MIN((size_t)size, AUDIO_BUF_SIZE(m_nSamplesPerFrame));
	UNALIGNED_MEMCPY(m_pAudioBuf, pData, m_nAudioBufLen);
	m_audioTimestamp = pFrame->getTimestamp();

	if ( m_videoTrackId == MP4_INVALID_TRACK_ID && logger_is_enabled(LT_DEBUG|L_MP4FILE_DYN) ) {
		int 	nFps = m_nAudioRate/m_nSamplesPerFrame;

		m_nDump++;
		if ( (m_nDump%(nFps*5)) == 0 ) {
			dumpFrames();
		}
	}

	return nresult;
}

result_t CMp4H264File::doWriteAudioFinalise()
{
	result_t		nresult = ESUCCESS;
#if 0
	if ( m_nAudioBufLen > 0 ) {
		MP4Duration		mp4Duration;
		bool			bResult;

		shell_assert(isOpen());
		shell_assert(m_audioTrackId != MP4_INVALID_TRACK_ID);
		shell_assert(m_pAudioBuf != NULL);
		shell_assert(m_audioTimestamp != TIMESTAMP_INVALID);

		mp4Duration = m_nAudioRate/m_nSamplesPerFrame; /* TODO */

		bResult = MP4WriteSample(m_hFile, m_audioTrackId, m_pAudioBuf,
								 m_nAudioBufLen, mp4Duration);
		if ( bResult ) {
			counter_inc(m_nAudioFrames);
		}
		else {
			nresult = EIO;
		}

		m_nAudioBufLen = 0;
	}
#endif
	return nresult;
}

result_t CMp4H264File::insertSubtitleTrack(uint32_t nRate, uint16_t nWidth, uint16_t nHeight)
{
	CAutoLock	locker(m_lock);
	result_t	nresult = ESUCCESS;

	//log_debug(L_MP4FILE, "[mp4_file] %s: inserting SUBTITLE track, %dx%d\n",
	//		  getFile(), nWidth, nHeight);

	if ( !isOpen() )  {
		log_error(L_MP4FILE, "[mp4_file] can't insert subtitle track, file is not open\n");
		return EFAULT;
	}

	if ( m_subtitleTrackId != MP4_INVALID_TRACK_ID )  {
		log_error(L_MP4FILE, "[mp4_file] %s: can't insert subtitle track: "
			"duplicated Subtitle track\n", getFile());
		return EINVAL;
	}

	shell_assert(m_nSubtitleRate == 0);
	shell_assert(m_nSubtitleBufLen == 0);

	if ( nRate == 0 )  {
		log_error(L_MP4FILE, "[mp4_file] %s: can't insert subtitle track: "
			"invalid time rate %d\n", getFile(), nRate);
		return EINVAL;
	}

	m_subtitleTrackId = MP4AddSubtitleTrack(m_hFile, nRate, nWidth, nHeight);
	if ( m_subtitleTrackId != MP4_INVALID_TRACK_ID ) {
		m_nSubtitleRate = nRate;
		m_subtitleTimestamp = TIMESTAMP_INVALID;
		counter_init(m_nSubtitleFrames);
	}
	else {
		log_error(L_MP4FILE, "[mp4_file] %s: can't create subtitle track\n", getFile());
		nresult = EIO;
	}

	return nresult;
}

result_t CMp4H264File::writeSubtitleFrame(CMediaFrame* pFrame)
{
	CAutoLock		locker(m_lock);
	const uint8_t*	pData;
	int 			size;
	MP4Duration		mp4Duration;
	bool			bResult;
	result_t		nresult = ESUCCESS;

	shell_assert(pFrame);

	if ( !isOpen() )  {
		return EFAULT;			/* File is not open */
	}

	if ( m_subtitleTrackId == MP4_INVALID_TRACK_ID )  {
		return EFAULT;			/* No subtitle track inserted/created */
	}

	pData = (const uint8_t*)pFrame->getData();
	size = (int)pFrame->getSize();

	if ( !pData || size == 0 )  {
		return ESUCCESS;
	}

	//log_debug(L_GEN, "**** Write Subtitle, %d bytes\n", size);

	if ( m_nSubtitleBufLen > 0 )  {
		/*
		 * Write previous audio frame
		 */
		shell_assert(m_subtitleTimestamp != TIMESTAMP_INVALID);

		mp4Duration = pFrame->getTimestamp()-m_subtitleTimestamp;

		bResult = MP4WriteSample(m_hFile, m_subtitleTrackId, m_subtitleBuf,
								 m_nSubtitleBufLen, mp4Duration);
		if ( bResult ) {
			counter_inc(m_nSubtitleFrames);
		}
		else {
			nresult = EIO;
		}

		m_nSubtitleBufLen = 0;
	}

	m_nSubtitleBufLen = (uint32_t)MIN((size_t)size, sizeof(m_subtitleBuf));
	UNALIGNED_MEMCPY(m_subtitleBuf, pData, m_nSubtitleBufLen);
	m_subtitleTimestamp = pFrame->getTimestamp();

	return nresult;
}


/*******************************************************************************
 * Debugging support
 */

void CMp4H264File::dump(const char* strPref) const
{
	CAutoLock	locker(m_lock);

	log_dump("*** %sFile: %s, open: %s, clock rate: %d\n", strPref,
			 m_strFilename.cs(), isOpen() ? "YES" : "NO", m_nTimeRate);

	if ( m_nVideoFps > 0 )  {
		log_dump("    Video: fps: %d, clock rate: %d, written %u frames\n",
				 m_nVideoFps, m_nVideoRate, counter_get(m_nVideoFrames));
	}
	else {
		log_dump("    Video: -NO TRACK-\n");
	}

	if ( m_nAudioRate > 0 )  {
		log_dump("    Audio: spf: %d, clock rate: %d, written %u frames\n",
				 m_nSamplesPerFrame, m_nAudioRate, counter_get(m_nAudioFrames));
	}
	else {
		log_dump("    Audio: -NO TRACK-\n");
	}

	log_dump("   Subtitle: %u frames\n", counter_get(m_nSubtitleFrames));
}

void CMp4H264File::dumpDyn() const
{
	CAutoLock	locker(m_lock);

	log_dump(">> Mp4file: %s, frames written: %u video, %u audio\n",
			 m_strFilename.cs(), counter_get(m_nVideoFrames),
			 counter_get(m_nAudioFrames));
}

void CMp4H264File::dumpFrames() const
{
	int 	x = (m_videoTrackId!=0) + (m_audioTrackId!=0)*2;

	switch ( x )  {
		case 1:
			log_dump(">> MP4File: %s: V: %u frame(s), S: %u frame(s)\n",
					 getFile(), counter_get(m_nVideoFrames), counter_get(m_nSubtitleFrames));
			break;

		case 2:
			log_dump(">> MP4File: %s: A: %u frame(s)\n",
					 getFile(), counter_get(m_nAudioFrames));
			break;

		case 3:
			log_dump(">> MP4File: %s: V: %u, A: %u frame(s), S: %u frame(s)\n",
					getFile(), counter_get(m_nVideoFrames), counter_get(m_nAudioFrames),
					counter_get(m_nSubtitleFrames));
			break;
	}
}
