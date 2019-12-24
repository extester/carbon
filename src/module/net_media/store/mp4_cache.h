/*
 *	Carbon/Network MultiMedia Streaming Module
 *	MP4 File writer cache
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 17.11.2016 12:52:50
 *	    Initial revision.
 */

#ifndef __MP4_CACHE_H_INCLUDED__
#define __MP4_CACHE_H_INCLUDED__

#include <map>
#include <mp4v2/mp4v2.h>

#include "shell/counter.h"
#include "carbon/carbon.h"
#include "carbon/thread.h"
#include "carbon/cstring.h"

#define MP4_CACHE_BLOCKSIZE			(512*1024)

class CMp4Cache;

class CMp4CacheItem
{
	private:
		CMp4Cache*		m_pParent;			/* Parent cache pointer */

		CString			m_strFile;			/* Open full filename */
		int				m_hHandle;			/* OPen file handle or -1 */
		uint8_t*		m_pBuffer;			/* Memory buffer */
		uint32_t		m_nSize;			/* Current data size in buffer */
		int64_t			m_nOffset;			/* Offset within buffer */
		counter_t		m_nErrors;			/* Error count within current file */

	public:
		CMp4CacheItem(const char* strFile, CMp4Cache* pParent);
		virtual ~CMp4CacheItem();

	public:
		result_t open(MP4FileMode fileMode);
		result_t seek(int64_t nPos);
		result_t read(void* pBuffer, int64_t size, int64_t* nin, int64_t maxChunkSize);
		result_t write(const void* pBuffer, int64_t size, int64_t* nout, int64_t maxChunkSize);
		result_t close();

		void dump(const char* strMargin) const;

	private:
		result_t doFlush();
		result_t doWrite(const void* pBuffer, int64_t size, int64_t off);
};

/*
 * MP4 file write cache class
 */
class CMp4Cache
{
	protected:
		size_t							m_nBlockSize;		/* Cache I/O block size */
		std::map<void*, CMp4CacheItem*>	m_file;				/* Cached files */

		static CMutex					m_lock;				/* Synchronisation access */
		static CMp4Cache*				m_pInstance;		/* Singleton */
		static const MP4FileProvider 	m_fileProvider;

	public:
		CMp4Cache(size_t nBlockSize);
		virtual ~CMp4Cache();

	public:
		size_t getCacheBufferSize() const { return m_nBlockSize; }
		void insertItem(CMp4CacheItem* pItem);
		void deleteItem(CMp4CacheItem* pItem);
		CMp4CacheItem* getItem(void* pHandle) const;

		void dump(const char* strPref = "") const;

		static CMp4Cache* getCache();
		static const MP4FileProvider* getFileProvider() { return &m_fileProvider; }

		static void* cacheOpen(const char* strFile, MP4FileMode fileMode);
		static int cacheSeek(void* pHandle, int64_t pos);
		static int cacheRead(void* pHandle, void* pBuffer, int64_t size, int64_t* nin, int64_t maxChunkSize);
		static int cacheWrite(void* pHandle, const void* pBuffer, int64_t size, int64_t* nout, int64_t maxChunkSize);
		static int cacheClose(void* pHandle);

};

#endif /* __MP4_CACHE_H_INCLUDED__ */
