/*
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
 *
 *  Revision 2.0, 02.02.2021 14:02:32
 *  	Fixed terminate(),
 *  	user two extra signals: SIGTSTP, SIGTTIN for thread termination
 */
/*
 * Note: appender is available if LOGGER_SINGLE_THREAD is disabled
 *
 */
/*
 * logger_addTcpServerAppender/logger_remove_appender()
 * must be called in context of the main thread
 * (in CApplication::init()/CApplication::terminate())
 */

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "shell/tstring.h"
#include "shell/unaligned.h"
#include "shell/hr_time.h"
#include "shell/logger/logger.h"
#include "shell/logger/appender_tcp_server.h"

#if !LOGGER_SINGLE_THREAD

#define TCP_SERVER_ITEM_MAX				4
#define TCP_SERVER_SENT_TIMEOUT			HR_1SEC

CAppenderTcpServer::CAppenderTcpServer(unsigned int nPort) :
	CAppender(),
	m_thAccept(0),
	m_thAppend(0),
	m_bTerminating(false),
	m_nPort(nPort)
{
	pthread_condattr_t	cond_attr;
	int					retVal;

	m_arItem.reserve(TCP_SERVER_ITEM_MAX);
	retVal = pthread_mutex_init(&m_lock, NULL);
	if ( retVal != 0 ) {
		CLogger::syslog("[AppenderTcpServer] mutex initialisation failed, result %d\n", errno);
	}

	pthread_condattr_init(&cond_attr);
	pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);

	retVal = pthread_cond_init(&m_cond, &cond_attr);
	if ( retVal != 0 ) {
		CLogger::syslog("[AppenderTcpServer] cond initialisation failed, result %d\n", errno);
	}

	pthread_condattr_destroy(&cond_attr);
}

CAppenderTcpServer::~CAppenderTcpServer()
{
	pthread_mutex_destroy(&m_lock);
	pthread_cond_destroy(&m_cond);
}

/*
 * Add new item to the connected client list
 *
 * 		hSocket		connected socket of the new client
 */
void CAppenderTcpServer::addItem(int hSocket)
{
	lock();

	if ( m_arItem.size() < TCP_SERVER_ITEM_MAX )  {
		tcp_server_item_t	item;

		_tbzero_object(item);
		item.hSocket = hSocket;

		m_arItem.push_back(item);
		unlock();
	}
	else {
		unlock();
		::close(hSocket);

		CLogger::syslog("[AppenderTcpServer] too many clients\n");
	}
}

/*
 * Delete an item from the connected client list
 *
 * 		pItem		deleting item
 */
void CAppenderTcpServer::deleteItem(tcp_server_item_t* pItem)
{
	size_t	i, count;

	lock();

	count = m_arItem.size();
	for(i=0; i<count; i++)  {
		if ( &m_arItem[i] == pItem )  {
			::close(pItem->hSocket);
			m_arItem.erase(m_arItem.begin()+i);
			break;
		}
	}

	unlock();
}

/*
 * Send a single string to the all appender's items
 *
 * 		pData		data to write
 * 		length		length of data, bytes
 *
 * Return: ESUCCESS, EIO
 */
result_t CAppenderTcpServer::append(const void* pData, size_t nLength)
{
	tcp_server_item_t*	pItem;
	int					successCount = 0, dropCount = 0;
	size_t				i, count, len;

	if ( nLength == 0 )  {
		return ESUCCESS;
	}

	lock();

	count = m_arItem.size();
	for(i=0; i<count; i++)  {
		pItem = &m_arItem[i];

		len = sizeof(pItem->strBuffer)-pItem->count;
		if ( len >= nLength )  {
			/* Send data to the item */
			UNALIGNED_MEMCPY(&pItem->strBuffer[pItem->count], pData, nLength);
			pItem->count += nLength;
			successCount ++;
		}
		else {
			dropCount ++;
		}
	}

	if ( successCount > 0 )  {
		pthread_cond_broadcast(&m_cond);
	}

	unlock();

	if ( dropCount > 0 )  {
		CLogger::syslog("[AppenderTcpServer] dropped message of the %d item(s)\n", dropCount);
	}

	return dropCount == 0 ? ESUCCESS : EBUSY;
}

/*
 * Send a data to the single connected client
 *
 * 		pItem		connected item
 *
 * Return: ESUCCESS, ETIMEDOUT,...
 */
result_t CAppenderTcpServer::doSend(tcp_server_item_t* pItem)
{
	struct pollfd       pfd[1];
	hr_time_t           hrStart;
	int                 timeout_ms, n;
	ssize_t             len;
	size_t              wSize;
	result_t			nresult;

	nresult = ESUCCESS;
	hrStart = hr_time_now();

	while ( nresult == ESUCCESS && pItem->sent < pItem->count ) {
		timeout_ms = (int)HR_TIME_TO_MILLISECONDS(TCP_SERVER_SENT_TIMEOUT-hr_time_get_elapsed(hrStart));
		if ( timeout_ms < 1 )  {
			nresult = ETIMEDOUT;
			break;
		}

		pfd[0].fd = pItem->hSocket;
		pfd[0].events = POLLOUT;
		pfd[0].revents = 0;

		unlock();
		n = ::poll(pfd, 1, timeout_ms);
		lock();

		if ( n == 0 )  {
			/* Timeout and no sockets are ready */
			nresult = ETIMEDOUT;
			break;
		}

		if ( n < 0 )  {
			/* Polling failed */
			nresult = errno;
			break;
		}

		if ( pfd[0].revents&POLLOUT )  {
			while( pItem->sent < pItem->count )  {
				wSize = pItem->count-pItem->sent;
				len = ::write(pfd[0].fd, &pItem->strBuffer[pItem->sent], wSize);
				if ( len < 0 )  {
					if ( errno != EAGAIN )  {
						nresult = errno;
					}
					break;
				}

				if ( len == 0 )  {
					break;
				}

				pItem->sent += len;
			}
		}

		if ( (pfd[0].revents&(POLLERR|POLLHUP|POLLNVAL)) != 0 && nresult == ESUCCESS )  {
			if ( (pfd[0].revents&POLLERR) != 0 && pItem->sent == 0 )  {
				nresult = EIO;
			} else
			if ( (pfd[0].revents&POLLHUP) != 0 && pItem->sent == 0 )  {
				nresult = ECONNRESET;
			} else
			if ( (pfd[0].revents&POLLHUP) != 0 && pItem->sent == 0 )  {
				nresult = EINVAL;
			}
		}
	}

	if ( pItem->sent == pItem->count )  {
		pItem->sent = pItem->count = 0;
	}

	return nresult;
}

void CAppenderTcpServer::setupSignals(int unblockSig)
{
	sigset_t        sigset;

	sigfillset(&sigset);
	pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);

	sigfillset(&sigset);
	sigdelset(&sigset, SIGQUIT);
	sigdelset(&sigset, SIGBUS);
	sigdelset(&sigset, unblockSig);
	sigdelset(&sigset, SIGFPE);
	sigdelset(&sigset, SIGILL);
	sigdelset(&sigset, SIGSEGV);
	pthread_sigmask(SIG_BLOCK, &sigset, NULL);
}

static void userHandler(int signum)
{
	(void)signum;
	//printf("\n*INTR*\n"); fflush(stdout);
}

result_t CAppenderTcpServer::setHandler(int signum, void (*handler)(int))
{
	struct sigaction	sa;
	result_t			nresult = ESUCCESS;

	_tbzero_object(sa);
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	if ( sigaction(signum, &sa, NULL) == -1 )  {
		nresult = errno;
		CLogger::syslog("[AppenderTcpServer]failed to setup signal handler %d, result: %d\n",
						signum, nresult);
	}

	return nresult;
}

/*
 * Appender worker thread
 */
void* CAppenderTcpServer::appendThread()
{
	size_t		i, count, n;
	boolean_t 	bRestart;
	result_t	nresult;

	setupSignals(SIGTTIN);
	setHandler(SIGTTIN, userHandler);

	while ( !m_bTerminating )  {
		lock();

		do {
			bRestart = false;
			n = 0;
			count = m_arItem.size();

			for(i=0; i<count; i++)  {
				if ( m_arItem[i].sent < m_arItem[i].count )  {
					nresult = doSend(&m_arItem[i]);
					if ( nresult != ESUCCESS )  {
						unlock();
						deleteItem(&m_arItem[i]);
						CLogger::syslog("[AppenderTcpServer] sent failure, result %d, client dropped\n", nresult);
						bRestart = true;
						lock();
						break;
					}
					n++;
				}
			}

		} while (bRestart && !m_bTerminating);

		if ( n == 0 && !m_bTerminating )  {
			pthread_cond_wait(&m_cond, &m_lock);
		}

		unlock();
	}

	return nullptr;
}

/*
 * Appender new client accepting thread
 */
void* CAppenderTcpServer::acceptThread()
{
	struct sockaddr_in 	sin;
	int					hSocket, retVal, n;

	setupSignals(SIGTSTP);
	setHandler(SIGTSTP, userHandler);


	hSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if ( hSocket < 0 )  {
		CLogger::syslog("[AppenderTcpServer] failed to create listen socket, result %d\n", errno);
		return nullptr;
	}

	n = 1;
	retVal = ::setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));
	if ( retVal < 0 )  {
		CLogger::syslog("[AppenderTcpServer] failed to set REUSEADDR to the listen socket, result %d\n", errno);
	}

	_tbzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(m_nPort);
	sin.sin_addr.s_addr = INADDR_ANY;

	retVal = ::bind(hSocket, (struct sockaddr*)&sin, sizeof(sin));
	if ( retVal < 0 )  {
		CLogger::syslog("[AppenderTcpServer] failed to bind listen socket, result %d\n", errno);
		::close(hSocket);
		return nullptr;
	}

	retVal = ::listen(hSocket, TCP_SERVER_ITEM_MAX);
	if ( retVal < 0 )  {
		CLogger::syslog("[AppenderTcpServer] failed to listen for listen socket, result %d\n", errno);
	}

	while ( !m_bTerminating )  {
		struct sockaddr_in	sockaddr;
		socklen_t 			sockaddr_len = sizeof(sockaddr);
		int					hNewSocket;

		hNewSocket = ::accept(hSocket, (struct sockaddr*)&sockaddr, &sockaddr_len);
		if ( hNewSocket >= 0 )  {
			/* New connection */
			int	flags;

			/* Set socket non-blocking mode */
			flags = ::fcntl(hNewSocket, F_GETFL, 0);
			if ( flags != -1 )  {
				retVal = ::fcntl(hNewSocket, F_SETFL, flags|O_NONBLOCK);
				if ( retVal != -1 )  {
					addItem(hNewSocket);
				}
				else {
					CLogger::syslog("[AppenderTcpServer] failed to set socket flags, result=%d\n", errno);
					::close(hNewSocket);
				}
			}
			else {
				CLogger::syslog("[AppenderTcpServer] failed to get socket fd flags, result=%d\n", errno);
				::close(hNewSocket);
			}
		}
		else {
			if ( errno != EINTR && !m_bTerminating )  {
				hr_sleep(HR_2SEC);
			}
		}
	}

	::close(hSocket);
	return nullptr;
}

/*
 * Appender initialisation
 *
 * Return: ESUCCESS, ...
 */
result_t CAppenderTcpServer::init()
{
	pthread_attr_t	attr;
	int				retVal;
	result_t		nresult = ESUCCESS;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

	retVal = pthread_create(&m_thAccept, &attr, acceptThreadST, this);
	if ( retVal != 0 ) {
		nresult = errno;
		CLogger::syslog("[AppenderTcpServer] failed to start accept thread, result %d\n", nresult);
	}

	retVal = pthread_create(&m_thAppend, &attr, appendThreadST, this);
	if ( retVal != 0 )  {
		nresult_join(nresult, errno);
		CLogger::syslog("[AppenderTcpServer] failed to start append thread, result %d\n", errno);
	}

	pthread_attr_destroy(&attr);

	return nresult;
}

/*
 * Release appender resources
 */
void CAppenderTcpServer::terminate()
{
	size_t	i, count;

	m_bTerminating = true;
	pthread_cond_broadcast(&m_cond);

	if ( m_thAccept )  {
		pthread_kill(m_thAccept, SIGTSTP);
		pthread_join(m_thAccept, nullptr);
		m_thAccept = 0;
	}

	if ( m_thAppend )  {
		pthread_kill(m_thAppend, SIGTTIN);
		pthread_join(m_thAppend, nullptr);
		m_thAppend = 0;
	}

	count = m_arItem.size();
	for(i=0; i<count; i++)  {
		::close(m_arItem[i].hSocket);
	}
	m_arItem.clear();
}

#endif /* !LOGGER_SINGLE_THREAD */
