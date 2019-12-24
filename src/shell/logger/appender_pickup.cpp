/*
 *  Shell library
 *  Pickup appender for the logger
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 26.08.2015 11:37:59
 *      Initial revision.
 */
/*
 * 		+-------------------+ 0
 * 		|					|
 * 		|					| <- m_nLastBlock
 * 		|					|
 * 		+-------------------+ PICKUP_APPENDER_DROP_SIZE
 * 		|					|
 * 		|					| <- m_nLastBlock
 * 		|					|
 * 		|					|
 * 		|					|
 * 		+-------------------+ PICKUP_APPENDER_MAX_SIZE
 *
 */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#if !LOGGER_SINGLE_THREAD
#include <pthread.h>
#endif /* !LOGGER_SINGLE_THREAD */

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/logger/appender_file.h"

#define PICKUP_APPENDER_MAX_SIZE			(1024*1024*200)		/* 200M */
#define PICKUP_APPENDER_DROP_SIZE			(1024*1024*50)		/* 50M */


class CAppenderPickup : public CAppenderFile
{
	private:
		size_t				m_szLastBlock;
#if !LOGGER_SINGLE_THREAD
		pthread_mutex_t		m_lock;			/* Appender lock */
#endif /* !LOGGER_SINGLE_THREAD */

	public:
		CAppenderPickup(const char* strFilename) :
			CAppenderFile(strFilename, 1, PICKUP_APPENDER_MAX_SIZE)
		{
#if !LOGGER_SINGLE_THREAD
			pthread_mutex_init(&m_lock, NULL);
#endif /* !LOGGER_SINGLE_THREAD */
		}

		virtual ~CAppenderPickup()
		{
#if !LOGGER_SINGLE_THREAD
			pthread_mutex_destroy(&m_lock);
#endif /* !LOGGER_SINGLE_THREAD */
		}

	public:
		virtual result_t init();
		virtual void terminate();
		virtual result_t append(const void* pData, size_t nLength);

		result_t getBlock(void* pBuffer, size_t* pSize);
		void getBlockConfirm(size_t size);
		void getBlockUndo(size_t size);

	private:
		void lock() {
#if !LOGGER_SINGLE_THREAD
			pthread_mutex_lock(&m_lock);
#endif /* !LOGGER_SINGLE_THREAD */
		}

		void unlock() {
#if !LOGGER_SINGLE_THREAD
			pthread_mutex_unlock(&m_lock);
#endif /* !LOGGER_SINGLE_THREAD */
		}

		result_t readBlock(void* pBuffer, size_t* pSize);
		result_t dropBlock(size_t size);
};


/*
 * Initialise an appender
 *
 * Return: ESUCCESS, ...
 */
result_t CAppenderPickup::init()
{
	result_t	nresult;

	nresult = CAppenderFile::init();
	m_szLastBlock = 0;
	return nresult;
}

/*
 * Free an appender resources
 */
void CAppenderPickup::terminate()
{
	CAppenderFile::terminate();
}

/*
 * Send a single string to the appender target
 *
 * 		pData		data to write
 * 		nLength		length of data, bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CAppenderPickup::append(const void* pData, size_t nLength)
{
	result_t	nresult;

	lock();

	nresult = logger_append_file(m_strFilename, pData, nLength);
	if ( nresult != ESUCCESS ) {
		unlock();
		return nresult;
	}

	m_nSize += nLength;
	if ( m_nSize > m_nFileSizeMax )  {
		nresult = dropBlock(PICKUP_APPENDER_DROP_SIZE);
		if ( nresult == ESUCCESS )  {
			m_nSize -= PICKUP_APPENDER_DROP_SIZE;
			if ( m_szLastBlock > 0 )  {
				m_szLastBlock = m_szLastBlock < PICKUP_APPENDER_DROP_SIZE ? 0 :
								(m_szLastBlock-PICKUP_APPENDER_DROP_SIZE);
			}
		}
	}

	unlock();

	return nresult;
}

/*
 * Read a logger file
 *
 * 		pBuffer			output buffer
 * 		pSize			IN: bytes to read
 * 						OUT: actual read bytes
 * Return: ESUCCESS, ...
 */
result_t CAppenderPickup::readBlock(void* pBuffer, size_t* pSize)
{
	int			handle;
	ssize_t		length, rlength;
	result_t	nresult = ESUCCESS;

	handle = ::open(m_strFilename, O_RDONLY);
	if ( handle >= 0 )  {
		length = ::read(handle, pBuffer, *pSize);
		if ( length > 0  ) {
			char* s = &((char*) pBuffer)[length - 1];

			rlength = length;
			for (; length > 0; length--) {
				if ( *s == '\n' || *s == '\r' ) {
					length = A(s) - A(pBuffer) + 1;
					break;
				}
				s--;
			}
			*pSize = (size_t)(length ? length : rlength);
		}
		else {
			if ( length == 0 ) {
				*pSize = 0;
			}
			else {
				nresult = errno;
				logger_syslog_impl("PICKUP APPENDER ERROR: failed to read file %s, result: %d\n",
								   m_strFilename, nresult);
			}
		}

		::close(handle);
	}
	else {
		nresult = errno;
		logger_syslog_impl("PICKUP APPENDER ERROR: failed to open %s, result: %d\n",
				m_strFilename, nresult);
	}

	return nresult;
}

#define FILE_BLOCK_SIZE		(1024*1024)

/*
 * Cut logger file
 *
 * 		size		bytes to cut from the beginning
 *
 * Return: ESUCCESS, ...
 */
result_t CAppenderPickup::dropBlock(size_t size)
{
	int			retVal, srcHandle, tmpHandle;
	void*		tmpBuffer;
	char		strTempFile[APPENDER_FILENAME_MAX+64];
	off_t		offset;
	ssize_t		count, wcount;
	result_t	nresult;

	if ( size == 0 )  {
		return ESUCCESS;
	}

	tmpBuffer = ::malloc(FILE_BLOCK_SIZE);
	if ( !tmpBuffer )  {
		logger_syslog_impl("PICKUP APPENDER ERROR: failed to allocate memory\n");
		return ENOMEM;
	}

	_tsnprintf(strTempFile, sizeof(strTempFile), "%s.tmp", m_strFilename);
	tmpHandle = ::open(strTempFile, O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
	if ( tmpHandle <= 0 )  {
		nresult = errno;
		logger_syslog_impl("PICKUP APPENDER ERROR: failed to create file %s, result: %d\n",
						   strTempFile, nresult);
		free(tmpBuffer);
		return nresult;
	}

	srcHandle = ::open(m_strFilename, O_RDONLY);
	if ( srcHandle < 0 )  {
		nresult = errno;
		logger_syslog_impl("PICKUP APPENDER ERROR: failed to open file %s, result: %d\n",
						   m_strFilename, nresult);
		::close(tmpHandle);
		free(tmpBuffer);
		return nresult;
	}

	offset = lseek(srcHandle, size, SEEK_SET);
	if ( offset == (off_t)-1 )  {
		nresult = errno;
		logger_syslog_impl("PICKUP APPENDER ERROR: seek failed, result: %d\n", nresult);
		::close(tmpHandle);
		::close(srcHandle);
		free(tmpBuffer);
		return nresult;
	}

	nresult = ESUCCESS;
	while ( nresult == ESUCCESS )  {
		count = ::read(srcHandle, tmpBuffer, FILE_BLOCK_SIZE);
		if ( count > 0 )  {
			wcount = ::write(tmpHandle, tmpBuffer, (size_t)count);
			if ( wcount != count )  {
				nresult = EIO;
				logger_syslog_impl("PICKUP APPENDER ERROR: write failed, result: %d\n", nresult);
			}
		}
		else
		if ( count == 0 ) {
			break;	/* eof */
		}
		else {
			nresult = errno;
			logger_syslog_impl("PICKUP APPENDER ERROR: read failed, result: %d\n", nresult);
		}
	}

	::close(tmpHandle);
	::close(srcHandle);
	free(tmpBuffer);

	if ( nresult == ESUCCESS )  {
		retVal = ::rename(strTempFile, m_strFilename);
		if ( retVal != 0 )  {
			nresult = errno;
			logger_syslog_impl("PICKUP APPENDER ERROR: failed to rename '%s' => '%s', result: %d\n",
							   strTempFile, m_strFilename, nresult);
		}
	}

	return nresult;
}

/*
 * Read a logger file and save read size
 *
 * 		pBuffer			buffer to read
 * 		pSize			IN: bytes to read
 * 						OUT: actual read bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CAppenderPickup::getBlock(void* pBuffer, size_t* pSize)
{
	size_t		szBlock;
	result_t	nresult;

	lock();

	if ( *pSize == 0 || m_nSize == 0 )  {
		m_szLastBlock = 0;
		*pSize = 0;
		unlock();
		return ESUCCESS;
	}

	szBlock = MIN(*pSize, m_nSize);
	nresult = readBlock(pBuffer, &szBlock);
	if ( nresult == ESUCCESS )  {
		*pSize = szBlock;
		m_szLastBlock = szBlock;
	}

	unlock();

	return nresult;
}

/*
 * Cut logger file
 *
 * 		size		cut bytes from the beginning
 */
void CAppenderPickup::getBlockConfirm(size_t size)
{
	lock();
	if ( m_szLastBlock > 0 )  {
		dropBlock(MIN(m_szLastBlock, size));
		m_szLastBlock = 0;
	}
	unlock();
}

/*
 * Undo previous getBlock() call
 *
 * 		size		block size, bytes
 */
void CAppenderPickup::getBlockUndo(size_t size)
{
	lock();
	m_szLastBlock = 0;
	unlock();
}


/*
 * Add PICKUP appender to the existing logger
 *
 *		strFilename			full filename
 *		nFileCountMax		maximum files to store
 *		nFileSizMax			maximum a log file size, bytes
 *
 * Return: appender handle or LOGGER_APPENDER_NULL
 */
appender_handle_t logger_addPickupAppender(const char* strFilename)
{
	CAppenderPickup*	pAppender;
	appender_handle_t	hAppender;

	pAppender = new CAppenderPickup(strFilename);
	hAppender = logger_insert_appender_impl(pAppender);
	if ( hAppender == LOGGER_APPENDER_NULL )  {
		/* Failed */
		delete pAppender;
	}

	return hAppender;
}

/*
 * Read a logger file and save read size
 *
 *		hAppender		appender handle
 * 		pBuffer			buffer to read
 * 		pSize			IN: bytes to read
 * 						OUT: actual read bytes
 *
 * Return: ESUCCESS, ...
 */
result_t logger_getPickupAppenderBlock(appender_handle_t hAppender,
									   void* pBuffer, size_t* pSize)
{
	CAppenderPickup*	pAppender;
	result_t			nresult = EINVAL;

	pAppender = dynamic_cast<CAppenderPickup*>(CAppender::fromHandle(hAppender));
	if ( pAppender )  {
		nresult = pAppender->getBlock(pBuffer, pSize);
	}

	return nresult;
}

/*
 * Remove previously read block from the logger
 *
 *		hAppender		appender handle
 *		size			block size
 */
void logger_getPickupAppenderBlockConfirm(appender_handle_t hAppender, size_t size)
{
	CAppenderPickup*	pAppender;

	pAppender = dynamic_cast<CAppenderPickup*>(CAppender::fromHandle(hAppender));
	if ( pAppender )  {
		pAppender->getBlockConfirm(size);
	}
}

/*
 * Undo logger_getPickupAppenderBlock() call
 *
 *		hAppender		appender handle
 *		size			block size
 */
void logger_getPickupAppenderBlockUndo(appender_handle_t hAppender, size_t size)
{
	CAppenderPickup*	pAppender;

	pAppender = dynamic_cast<CAppenderPickup*>(CAppender::fromHandle(hAppender));
	if ( pAppender )  {
		pAppender->getBlockUndo(size);
	}
}
