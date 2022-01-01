/*
 *  Shell library
 *  TSL/SSL TCP sockets
 *
 *  Copyright (c) 2021 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 22.12.2021 19:46:40
 *      Initial revision.
 *
 */

#ifndef __SHELL_SSL_SOCKET_H_INCLUDED__
#define __SHELL_SSL_SOCKET_H_INCLUDED__

#include "shell/config.h"
#include "shell/socket.h"
#include "shell/openssl.h"

/*
 * Asynchronous SSL socket class
 */
class CSslSocketAsync : public CSocket
{
    protected:
		SSL_CTX*			m_pSslCtx;			/* SSL context (configuration data) */
		SSL*				m_pSslHandle;		/* SSL connectiom handle */
		const boolean_t		m_bClient;			/* true: socket in client state, false: server state */

		boolean_t			m_bTcpConnected;	/* true: tcp socket conneted (valid after
 											 	 * connectAsync()) */

	public:
		CSslSocketAsync(boolean_t bClient, int exOption = 0);
		virtual ~CSslSocketAsync();

	public:
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

		virtual result_t close();

		virtual result_t sendAsync(const void* pBuffer, size_t* pSize,
							   		const CNetAddr& destAddr = NETADDR_NULL);

		virtual result_t receiveAsync(void* pBuffer, size_t* pSize, CNetAddr* pSrcAddr = NULL);
		virtual result_t receiveLineAsync(void* pBuffer, size_t nPreSize, size_t* pSize,
									  		const char* strEol);

	protected:
		result_t sslError2nresult(int nSslError, const char** strError) const;
		result_t createSslContext();
		void deleteSslContext();
		result_t createSslHandle();
		void deleteSslHandle();

		virtual result_t connectSslAsync();
};

/*
 * Synchronous TLS/SSL socket class
 */
class CSslSocket : public CSslSocketAsync
{
	public:
		CSslSocket(boolean_t bClient, int exOption = 0);
		virtual ~CSslSocket();

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
};

#endif /* __SHELL_SSL_SOCKET_H_INCLUDED__ */
