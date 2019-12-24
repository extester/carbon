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

#include <fcntl.h>
#include <unistd.h>

#include "carbon/memory.h"

#include "net_media/store/mp4_cache.h"

/*******************************************************************************
 * CMp4CacheItem class
 */

CMp4CacheItem::CMp4CacheItem(const char* strFile, CMp4Cache* pParent) :
	m_pParent(pParent),
	m_strFile(strFile),
	m_hHandle(-1),
	m_pBuffer(NULL),
	m_nSize(0),
	m_nOffset(0),
	m_nErrors(ZERO_COUNTER)
{
}

CMp4CacheItem::~CMp4CacheItem()
{
	shell_assert(m_hHandle == -1);
}

result_t CMp4CacheItem::doWrite(const void* pBuffer, int64_t size, int64_t off)
{
	ssize_t		n;
	result_t	nresult = ESUCCESS;

	shell_assert(m_hHandle >= 0);

	n = ::pwrite(m_hHandle, pBuffer, size, off);
	if ( n < 0 )  {
		nresult = errno;
		log_error(L_MP4FILE, "[mp4_cache] failed to write file %s, %d bytes, result: %d\n",
				  m_strFile.cs(), (int)size, nresult);
	}
	else {
		if ( size != n )  {
			log_error(L_MP4FILE, "[mp4_cache] failed to write file %s, written %ld of %ld bytes\n",
					  m_strFile.cs(), n, size);
			nresult = EIO;
		}
	}

	return nresult;
}

result_t CMp4CacheItem::doFlush()
{
	result_t	nresult = ESUCCESS;

	if ( m_nSize > 0 )  {
		nresult = doWrite(m_pBuffer, m_nSize, m_nOffset);
	}

	return nresult;

}

result_t CMp4CacheItem::open(MP4FileMode fileMode)
{
	int 		handle;
	result_t	nresult;

	shell_assert(m_hHandle == -1);
	shell_assert(m_pBuffer == NULL);

	if ( m_strFile.isEmpty() )  {
		log_debug(L_MP4FILE, "[mp4_cache] no filename specified\n");
		return ENOENT;
	}

	m_pBuffer = (uint8_t*)memAlloc(m_pParent->getCacheBufferSize());
	if ( !m_pBuffer )  {
		log_error(L_MP4FILE, "[mp4_cache] memory allocation failed %u bytes, file: %s\n",
				  m_pParent->getCacheBufferSize(), m_strFile.cs());
		return ENOMEM;
	}

	handle = ::open(m_strFile, O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if ( handle < 0 )  {
		nresult = errno;
		log_error(L_MP4FILE, "[mp4_cache] failed to open file %s, result: %d\n",
				  m_strFile.cs(), nresult);
		SAFE_FREE(m_pBuffer);
		return nresult;
	}

	m_hHandle = handle;
	m_nOffset = 0;
	m_nSize = 0;
	counter_set(m_nErrors, 0);

	return ESUCCESS;
}

result_t CMp4CacheItem::seek(int64_t nPos)
{
	result_t	nresult;
	int			retVal = 0;

	if ( (m_nOffset+m_nSize) != nPos )  {
		nresult = doFlush();
		m_nOffset = nPos;
		m_nSize = 0;

		if ( nresult != ESUCCESS )  {
			retVal = 1;
			counter_inc(m_nErrors);
		}
	}

	return retVal;
}

result_t CMp4CacheItem::read(void* pBuffer, int64_t size, int64_t* nin, int64_t maxChunkSize)
{
	log_debug(L_MP4FILE, "[mp4_cache] file %s: read() is not impletented\n", m_strFile.cs());
	counter_inc(m_nErrors);
	return EIO;
}

result_t CMp4CacheItem::write(const void* pBuffer, int64_t size, int64_t* nout,
							  int64_t maxChunkSize)
{
	const uint8_t*	p = (const uint8_t*)pBuffer;
	int				i, count, rest, length = (int)size, l;
	size_t			nBlockSize = m_pParent->getCacheBufferSize();
	result_t		nresult = ESUCCESS, nr;

	l = MIN(length, (int)(nBlockSize-m_nSize));
	if ( l > 0 )  {
		UNALIGNED_MEMCPY(&m_pBuffer[m_nSize], p, l);
		length -= l;
		m_nSize += l;
		p += l;
	}

	if ( m_nSize == nBlockSize )  {
		nresult = doFlush();
		m_nOffset += nBlockSize;
		m_nSize = 0;

		if ( nresult != ESUCCESS )  {
			counter_inc(m_nErrors);
		}
	}

	if ( length == 0 )  {
		*nout = size;
		return nresult;
	}

	count = length/(int)nBlockSize;
	rest = length%(int)nBlockSize;

	for(i=0; i<count; i++)  {
		nr = doWrite(p, nBlockSize, m_nOffset);
		m_nOffset += nBlockSize;
		p += nBlockSize;

		if ( nr != ESUCCESS )  {
			counter_inc(m_nErrors);
			nresult_join(nresult, nr);
		}
	}

	if ( rest > 0 )  {
		UNALIGNED_MEMCPY(m_pBuffer, p, rest);
		m_nSize += rest;
	}

	*nout = size;
	return nresult;
}

result_t CMp4CacheItem::close()
{
	result_t	nresult;
	int 		retVal;

	nresult = doFlush();
	m_nOffset += m_nSize;
	m_nSize = 0;

	if ( nresult != ESUCCESS )  {
		counter_inc(m_nErrors);
	}

	retVal = ::close(m_hHandle);
	if ( retVal != 0 ) {
		nresult = errno;
		counter_inc(m_nErrors);
	}

	m_hHandle = -1;
	SAFE_FREE(m_pBuffer);

	return nresult;
}

/*******************************************************************************
 * Debugging support
 */

/*
 * Note: MP4 Cache must be locked
 */
void CMp4CacheItem::dump(const char* strMargin) const
{
	log_dump("%sItem: %s, open: %s, off: %u, size: %u, errors: %d\n",
			 m_strFile.cs(), m_hHandle != -1 ? "YES" : "NO",
			 (int)m_nOffset, m_nSize, counter_get(m_nErrors));
}

/*******************************************************************************
 * CMp4Cache class
 */

CMutex CMp4Cache::m_lock;
CMp4Cache* CMp4Cache::m_pInstance = 0;
const MP4FileProvider CMp4Cache::m_fileProvider = {
	CMp4Cache::cacheOpen,
	CMp4Cache::cacheSeek,
	CMp4Cache::cacheRead,
	CMp4Cache::cacheWrite,
	CMp4Cache::cacheClose
};

/*
 * Get singleton instance
 */
CMp4Cache* CMp4Cache::getCache()
{
	CAutoLock	locker(m_lock);

	if ( m_pInstance == 0 )  {
		m_pInstance = new CMp4Cache(MP4_CACHE_BLOCKSIZE);
	}
	return m_pInstance;
}

CMp4Cache::CMp4Cache(size_t nBlockSize) :
	m_nBlockSize(nBlockSize)
{
}

CMp4Cache::~CMp4Cache()
{
	shell_assert(m_file.size() == 0);
}

void CMp4Cache::insertItem(CMp4CacheItem* pItem)
{
	CAutoLock		locker(m_lock);
	std::pair<std::map<void*, CMp4CacheItem*>::iterator, bool>	result;

	result = m_file.insert(std::pair<void*, CMp4CacheItem*>(pItem, pItem));
	shell_assert(result.second != false); UNUSED(result);
}

void CMp4Cache::deleteItem(CMp4CacheItem* pItem)
{
	CAutoLock		locker(m_lock);
	std::map<void*, CMp4CacheItem*>::iterator it;

	it = m_file.find(pItem);
	if ( it != m_file.end() )  {
		delete pItem;
		m_file.erase(it);
	}
	else {
		locker.unlock();
		log_debug(L_MP4FILE, "[mp4_cache] cache item %lld is not found\n",
				  (unsigned long long)pItem);
	}
}

CMp4CacheItem* CMp4Cache::getItem(void* pHandle) const
{
	CAutoLock		locker(m_lock);
	std::map<void*, CMp4CacheItem*>::const_iterator it;
	CMp4CacheItem*	pItem = 0;

	it = m_file.find(pHandle);
	if ( it != m_file.end() )  {
		pItem = it->second;
	}
	else {
		locker.unlock();
		log_debug(L_MP4FILE, "[mp4_cache] cache item %lld is not found\n",
				  (unsigned long long)pHandle);
	}

	return pItem;
}

/*
 * [File Provider function]
 *
 * Open a new file
 *
 * 		strFile			full filename
 * 		fileMode		open mode (unused so far)
 *
 * Return:
 * 		0		file is not open
 * 		other	file was open, return descriptor
 */
void* CMp4Cache::cacheOpen(const char* strFile, MP4FileMode fileMode)
{
	CMp4Cache*		pCache = getCache();
	CMp4CacheItem*	pItem;
	result_t		nresult;

	pItem = new CMp4CacheItem(strFile, pCache);
	nresult = pItem->open(fileMode);
	if ( nresult == ESUCCESS )  {
		pCache->insertItem(pItem);
	}
	else {
		delete pItem;
	}

	return nresult == ESUCCESS ? pItem : NULL;
}

/*
 * [File Provider function]
 *
 * Seek file to a given position
 *
 * 		pHandle			file handle
 * 		nPos			file pointer position (offset from beginning)
 *
 * Return:
 * 		0		success
 * 		1		error
 */
int CMp4Cache::cacheSeek(void* pHandle, int64_t nPos)
{
	CMp4CacheItem*	pItem;
	result_t		nresult;
	int				retVal = 1;

	pItem = getCache()->getItem(pHandle);
	if ( pItem )  {
		nresult = pItem->seek(nPos);
		retVal = nresult == ESUCCESS ? 0 : 1;
	}

	return retVal;
}

/*
 * [File Provider function]
 *
 * Read file
 *
 * 		pHandle			file handle
 * 		pBuffer			buffer to read
 * 		size			read bytes
 * 		nin				???
 * 		maxChunkSize	???
 *
 * Return:
 * 		0		success
 * 		1		error
 */
int CMp4Cache::cacheRead(void* pHandle, void* pBuffer, int64_t size, int64_t* nin,
						 int64_t maxChunkSize)
{
	CMp4CacheItem*	pItem;
	int				retVal = 1;
	result_t		nresult;

	pItem = getCache()->getItem(pHandle);
	if ( pItem )  {
		nresult = pItem->read(pBuffer, size, nin, maxChunkSize);
		retVal = nresult == ESUCCESS ? 0 : 1;
	}
	else {
		*nin = 0;
	}

	return retVal;
}

/*
 * [File Provider function]
 *
 * Read write
 *
 * 		pHandle			file handle
 * 		pBuffer			buffer to write
 * 		size			write bytes
 * 		nout			???
 * 		maxChunkSize	???
 *
 * Return:
 * 		0		success
 * 		1		error
 */
int CMp4Cache::cacheWrite(void* pHandle, const void* pBuffer, int64_t size, int64_t* nout,
						  int64_t maxChunkSize)
{
	CMp4CacheItem*	pItem;
	int				retVal = 1;
	result_t		nresult;

	pItem = getCache()->getItem(pHandle);
	if ( pItem )  {
		nresult = pItem->write(pBuffer, size, nout, maxChunkSize);
		retVal = nresult == ESUCCESS ? 0 : 1;
	}
	else {
		*nout = 0;
	}

	return retVal;
}

/*
 * [File Provider function]
 *
 * Close file
 *
 * 		pHandle			file handle
 *
 * Return:
 * 		0		success
 * 		1		error
 */
int CMp4Cache::cacheClose(void* pHandle)
{
	CMp4Cache*		pCache = getCache();
	CMp4CacheItem*	pItem;
	int				retVal = 1;
	result_t		nresult;

	pItem = pCache->getItem(pHandle);
	if ( pItem )  {
		nresult = pItem->close();
		retVal = nresult == ESUCCESS ? 0 : 1;
		pCache->deleteItem(pItem);
	}

	return retVal;
}

/*******************************************************************************
 * Debugging support
 */

void CMp4Cache::dump(const char* strPref) const
{
	CAutoLock		locker(m_lock);
	std::map<void*, CMp4CacheItem*>::const_iterator it;

	log_dump("*** %sMP4 Cache (%u items):\n", m_file.size());

	for(it=m_file.begin(); it!=m_file.end(); it++) {
		it->second->dump("    ");
	}
}
