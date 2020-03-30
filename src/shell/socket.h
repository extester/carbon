/*
 *  Shell library
 *  TCP and UNIX sockets
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 30.01.2015 18:34:35
 *      Initial revision.
 *
 *  Revision 1.1, 04.04.2015 23:21:12
 *  	Added various connect() functions;
 *  	Added readCorrect() function (to be renamed to read()).
 *
 *  Revision 1.2, 30.03.2020 18:16:52
 *  	Renamed CSocketAsync: connect() => connectAsync(),
 *  	send() => sendAsync(), receive() => receiveAsync().
 *
 */

#ifndef __SHELL_SOCKET_H_INCLUDED__
#define __SHELL_SOCKET_H_INCLUDED__

#include <netinet/in.h>

#include "shell/config.h"
#include "shell/hr_time.h"
#include "shell/netaddr.h"
#include "shell/breaker.h"
#include "shell/ref_object.h"

#if CARBON_DEBUG_TRACK_OBJECT
#include "shell/track_object.h"
#endif /* CARBON_DEBUG_TRACK_OBJECT */

#define INPORT_ANY			0

typedef enum {
	SOCKET_TYPE_STREAM = SOCK_STREAM,
	SOCKET_TYPE_UDP = SOCK_DGRAM,
	SOCKET_TYPE_RAW = SOCK_RAW,

	SOCKET_TYPE_NONBLOCK = SOCK_NONBLOCK,
	SOCKET_TYPE_CLOEXEC = SOCK_CLOEXEC
} socket_type_t;


/*
 * Asynchronous socket class
 */
class CSocketAsync
{
    protected:
		int				m_hSocket;				/* Socket file descriptor */
		int				m_exOption;

	public:
		explicit CSocketAsync(int exOption = 0) : m_hSocket(-1), m_exOption(exOption) {}
		virtual ~CSocketAsync() { close(); }

	public:
		int getHandle() const { return m_hSocket; }
		boolean_t isOpen() const { return m_hSocket != -1; }

		virtual result_t open(const CNetAddr& bindAddr = NETADDR_NULL,
							  	socket_type_t sockType = SOCKET_TYPE_STREAM);
		virtual result_t open(const char* strBindSocket = NULL,
							  	socket_type_t sockType = SOCKET_TYPE_STREAM);

		virtual result_t connectAsync(const CNetAddr& dstAddr, const CNetAddr& bindAddr,
								 socket_type_t sockType = SOCKET_TYPE_STREAM);
		virtual result_t connectAsync(const CNetAddr& dstAddr,
								 socket_type_t sockType = SOCKET_TYPE_STREAM) {
			return connectAsync(dstAddr, NETADDR_NULL, sockType);
		}

		virtual result_t connectAsync(const char* strDstSocket, const char* strBindSocket,
								 	socket_type_t sockType = SOCKET_TYPE_STREAM);
		virtual result_t connectAsync(const char* strSocket, socket_type_t sockType = SOCKET_TYPE_STREAM) {
			return connectAsync(strSocket, NULL, sockType);
		}

		virtual result_t close();

		virtual result_t sendAsync(const void* pBuffer, size_t* pSize,
								const CNetAddr& destAddr = NETADDR_NULL);
		virtual result_t receiveAsync(void* pBuffer, size_t* pSize, CNetAddr* pSrcAddr = NULL);
		virtual result_t receiveLineAsync(void* pBuffer, size_t nPreSize, size_t* pSize,
								const char* strEol);

		CNetAddr getAddr() const;

		result_t setOption(int option, const void* pValue, int valueLen);
		result_t setOption(int option, int value) {
			return setOption(option, &value, sizeof(value));
		}

		result_t getOption(int option, void* pValue, int* pValueLength) const;
		result_t getOption(int option, int* pValue) const {
			int 	len = sizeof(*pValue);
			*pValue = 0;
			return getOption(option, pValue, &len);
		}

		result_t getError(int* pError) const;

	protected:
		result_t setOption(int fd, int option, int value);
		void setHandle(int fd) { close(); m_hSocket = fd; }
};

/*
 * Synchronous socket class
 */
class CSocket : public CSocketAsync
{
	protected:
		CFileBreaker	m_breaker;

    public:
		enum {
			noReuseAddr = 1,
			noReusePort = 2,
			broadcast = 4,			/* enable broadcast mode for the socket */
			readFull = 8,			/* receive(): receive a specified byte count */
			pollRead = 16,			/* select(): waits for read */
			pollWrite = 32			/* select(): waits for write */
		};

	public:
		explicit CSocket(int exOption = 0) : CSocketAsync(exOption) {}
		virtual ~CSocket() {}

	public:
		virtual result_t connect(const CNetAddr& dstAddr, hr_time_t hrTimeout,
								 const CNetAddr& bindAddr/* = NETADDR_NULL*/,
								 socket_type_t sockType = SOCKET_TYPE_STREAM);
		virtual result_t connect(const CNetAddr& dstAddr, hr_time_t hrTimeout,
							 socket_type_t sockType = SOCKET_TYPE_STREAM) {
			return connect(dstAddr, hrTimeout, NETADDR_NULL, sockType);
		}

		virtual result_t connect(const char* strDstSocket, hr_time_t hrTimeout,
								 const char* strBindSocket/* = NULL*/,
								 socket_type_t sockType = SOCKET_TYPE_STREAM);
		virtual result_t connect(const char* strDstSocket, hr_time_t hrTimeout,
							 socket_type_t sockType = SOCKET_TYPE_STREAM) {
			return connect(strDstSocket, hrTimeout, NULL, sockType);
		}

		virtual result_t send(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout,
								const CNetAddr& destAddr = NETADDR_NULL);
		virtual result_t send(const void* pBuffer, size_t size, hr_time_t hrTimeout,
							  	const CNetAddr& destAddr = NETADDR_NULL)  {
			return send(pBuffer, &size, hrTimeout, destAddr);
		}

		virtual result_t receive(void* pBuffer, size_t* pSize, int options,
								hr_time_t hrTimeout, CNetAddr* pSrcAddr = NULL);
		virtual result_t receiveLine(void* pBuffer, size_t* pSize, const char* strEol,
								hr_time_t hrTimeout);

		virtual result_t listen(int count);
		virtual result_t select(hr_time_t hrTimeout, int options, short* prevents = 0);
		virtual CSocket* accept();

		result_t breakerEnable() {
			return m_breaker.enable();
		}
		result_t breakerDisable() {
			return m_breaker.disable();
		}
		void breakerBreak() {
			m_breaker._break();
		}
		void breakerReset() {
			m_breaker.reset();
		}

	protected:
		result_t checkREvents(short revents) const;
};

#if CARBON_DEBUG_TRACK_OBJECT
#define __CSocketRef_PARENT				,public CTrackObject<CSocketRef>
#else /* CARBON_DEBUG_TRACK_OBJECT */
#define __CSocketRef_PARENT
#endif /* CARBON_DEBUG_TRACK_OBJECT */

/*
 * Socket class with reference counter
 */
class CSocketRef : public CSocket, public CRefObject __CSocketRef_PARENT
{
	public:
		explicit CSocketRef(int exOption = 0) :
			CSocket(exOption),
			CRefObject()/*,
			CTrackObject<CSocketRef>()*/
		{
		}

	protected:
		virtual ~CSocketRef()
		{
		}

	public:
		virtual CSocketRef* accept();

#if CARBON_DEBUG_DUMP
		virtual void dump(const char* strPref = "") const;
#else /* CARBON_DEBUG_DUMP */
		virtual void dump(const char* strPref = "") const { shell_unused(strPref); }
#endif /* CARBON_DEBUG_DUMP */
};


#endif /* __SHELL_SOCKET_H_INCLUDED__ */
