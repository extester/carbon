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

#ifndef __SHELL_LOGGER_APPENDER_PICKUP_H_INCLUDED__
#define __SHELL_LOGGER_APPENDER_PICKUP_H_INCLUDED__

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>

#if !LOGGER_SINGLE_THREAD
#include <pthread.h>
#endif /* !LOGGER_SINGLE_THREAD */

#include "shell/logger/logger.h"
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
		CAppenderPickup(const char* strFilename);
		virtual ~CAppenderPickup();

	public:
		virtual result_t init();
		virtual void terminate();
		virtual result_t append(const void* pData, size_t nLength);

		result_t getBlock(void* pBuffer, size_t* pSize);
		void getBlockConfirm(size_t nLength);
		void getBlockUndo(size_t nLength);

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

#endif /* __SHELL_LOGGER_APPENDER_PICKUP_H_INCLUDED__ */
