#/*
 *  Shell library
 *  TCP server appender for the logger
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.08.2015 17:28:25
 *      Initial revision.
 */
/*
 * Note: appender is available if LOGGER_SINGLE_THREAD is disabled
 */
/*
 * logger_addTcpServerAppender/logger_remove_appender()
 * must be called in context of the main thread
 * (in CApplication::init()/CApplication::terminate())
 */

#ifndef __SHELL_LOGGER_APPENDER_TCP_SERVER_H_INCLUDED__
#define __SHELL_LOGGER_APPENDER_TCP_SERVER_H_INCLUDED__

#include <vector>
#include <pthread.h>

#include "shell/config.h"

#include "shell/logger/appender.h"
#include "shell/logger/logger.h"

#if !LOGGER_SINGLE_THREAD

#define LOGGER_TCP_BUFFER_MAX			65536		/* Per connection buffer size */

class CAppenderTcpServer : public CAppender
{
	protected:
		struct tcp_server_item_t {
			int			hSocket;
			char		strBuffer[LOGGER_TCP_BUFFER_MAX];
			size_t		sent, count;
		};

	private:
		std::vector<tcp_server_item_t>	m_arItem;		/* Connected client list */
		pthread_t						m_thAccept;		/* Accepting new connection thread */
		pthread_t						m_thAppend;		/* Worker thread */
		pthread_mutex_t					m_lock;			/* Appender lock */
		pthread_cond_t 					m_cond;
		boolean_t						m_bTerminating;	/* TRUE: appender is terminating */
		unsigned int 					m_nPort;		/* Port to listen on */

	public:
		CAppenderTcpServer(unsigned int nPort);
		virtual ~CAppenderTcpServer();

	public:
		virtual result_t init();
		virtual void terminate();
		virtual result_t append(const void* pData, size_t nLength);

	private:
		void addItem(int hSocket);
		void deleteItem(tcp_server_item_t* pItem);
		result_t doSend(tcp_server_item_t* pItem);

		void lock() { pthread_mutex_lock(&m_lock); }
		void unlock() { pthread_mutex_unlock(&m_lock); }
		void setupSignals(int unblockSig);
		result_t setHandler(int signum, void (*handler)(int));

		static void* acceptThreadST(void* p) {
			return static_cast<CAppenderTcpServer*>(p)->acceptThread();
		}
		void* acceptThread();

		static void* appendThreadST(void* p) {
			return static_cast<CAppenderTcpServer*>(p)->appendThread();
		}
		void* appendThread();
};

#endif /* !LOGGER_SINGLE_THREAD */

#endif /* __SHELL_LOGGER_APPENDER_TCP_SERVER_H_INCLUDED__ */