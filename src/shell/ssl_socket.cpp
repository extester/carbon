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
 *  Revision 1.0, 22.12.2021 19:51:20
 *      Initial revision.
 *
 */

#include <poll.h>

#include "shell/logger.h"
#include "shell/ssl_socket.h"

/*******************************************************************************
 * Asynchronous TLS/SSL socket I/O class
 */

/*
 * Construct a socket object
 *
 * 		bClient			true: use socket in client mode, false: use socket in server mode
 * 		exOption		additional options (see CSocket)
 */
CSslSocketAsync::CSslSocketAsync(boolean_t bClient, int exOption) :
	CSocket(exOption),
	m_pSslCtx(nullptr),
	m_pSslHandle(nullptr),
	m_bClient(bClient),
	m_bTcpConnected(false)
{
}

CSslSocketAsync::~CSslSocketAsync()
{
	close();
}

/*
 * Convert openSSL error code to nresult
 *
 * 		nSslError			openssl error code
 * 		strError			out pointer to symbol error representation (optional, can be nullptr)
 *
 * Return: nresult error code
 */
result_t CSslSocketAsync::sslError2nresult(int nSslError, const char** strError) const
{
	result_t	nresult;

	switch ( nSslError )  {
		case SSL_ERROR_NONE:
			nresult = ESUCCESS;
			if ( strError )  *strError = "SSL_ERROR_NONE";
			break;

		case SSL_ERROR_ZERO_RETURN:
			nresult = ECONNRESET;
			if ( strError )  *strError = "SSL_ERROR_ZERO_RETURN";
			break;

		case SSL_ERROR_WANT_READ:
			nresult = EINPROGRESS;
			if ( strError )  *strError = "SSL_ERROR_WANT_READ";
			break;

		case SSL_ERROR_WANT_WRITE:
			nresult = EINPROGRESS;
			if ( strError )  *strError = "SSL_ERROR_WANT_WRITE";
			break;

		case SSL_ERROR_WANT_CONNECT:
			nresult = EINPROGRESS;
			if ( strError )  *strError = "SSL_ERROR_WANT_CONNECT";
			break;

		case SSL_ERROR_WANT_ACCEPT:
			nresult = EINPROGRESS;
			if ( strError )  *strError = "SSL_ERROR_WANT_ACCEPT";
			break;

		case SSL_ERROR_WANT_X509_LOOKUP:
			nresult = EINPROGRESS;
			if ( strError )  *strError = "SSL_ERROR_WANT_X509_LOOKUP";
			break;

		case SSL_ERROR_WANT_ASYNC:
			nresult = EINPROGRESS;
			if ( strError )  *strError = "SSL_ERROR_WANT_ASYNC";
			break;

		case SSL_ERROR_WANT_ASYNC_JOB:
			nresult = EINPROGRESS;
			if ( strError )  *strError = "SSL_ERROR_WANT_ASYNC_JOB";
			break;

		case SSL_ERROR_WANT_CLIENT_HELLO_CB:
			nresult = EINPROGRESS;
			if ( strError )  *strError = "SSL_ERROR_WANT_CLIENT_HELLO_CB";
			break;

		case SSL_ERROR_SYSCALL:
			nresult = EIO;
			if ( strError )  *strError = "SSL_ERROR_SYSCALL";
			break;

		case SSL_ERROR_SSL:
			nresult = EIO;
			if ( strError )  *strError = "SSL_ERROR_SSL";
			break;

		default:
			log_error(L_SOCKET, "[ssl_socket] unknown error code %d\n", nSslError);
			if ( strError )  *strError = "*UNKNOWN_ERROR*";
			nresult = EINVAL;
			break;
	}

	return nresult;
}

/*
 * Allocate and configure SSL context
 *
 * Return:
 * 		ESUCCESS		success, context has been creted
 * 		...				fatal error
 */
result_t CSslSocketAsync::createSslContext()
{
	result_t	nresult = ESUCCESS;

	if ( m_pSslCtx == nullptr )  {
		ERR_clear_error();
		m_pSslCtx = SSL_CTX_new(m_bClient ? TLS_client_method() : TLS_server_method());

		if ( m_pSslCtx == nullptr )  {
			const char*		strError;
			int				nSslError = ERR_get_error();

			nresult = sslError2nresult(nSslError, &strError);
			log_error(L_SOCKET, "[ssl_socket] can't create ssl context, ssl error %d (%s)\n",
					  				nSslError, strError);
		}
	}

	return nresult;
}

/*
 * Free SSL context if any
 */
void CSslSocketAsync::deleteSslContext()
{
	if ( m_pSslCtx != nullptr )  {
		SSL_CTX_free(m_pSslCtx);
		m_pSslCtx = nullptr;
	}
}

/*
 * Create a new SSL handle for the opened TCP socket
 *
 * Return:
 * 		ESUCCESS			socket created
 * 		...					fatal error
 *
 * Note: TCP socket must be opened/connected
 */
result_t CSslSocketAsync::createSslHandle()
{
	result_t		nresult = EINVAL;
	int				nSslResult, nSslError;
	const char*		strError;

	if ( m_pSslHandle == nullptr )  {
		boolean_t 	bSslCtx = m_pSslCtx != nullptr;

		shell_assert(isOpen());

		nresult = createSslContext();
		if ( nresult == ESUCCESS )  {
			ERR_clear_error();
			m_pSslHandle = SSL_new(m_pSslCtx);
			if ( m_pSslHandle != nullptr )  {
				ERR_clear_error();
				nSslResult = SSL_set_fd(m_pSslHandle, m_hSocket);
				if ( nSslResult != 1 )  {
					nSslError = SSL_get_error(m_pSslHandle, nSslResult);
					nresult = sslError2nresult(nSslError, &strError);

					log_error(L_SOCKET, "[ssl_socket] can't set ssl descriptor, ssl error %d (%s)\n",
							  				nSslError, strError);
					deleteSslHandle();
					if ( !bSslCtx )  {
						deleteSslContext();
					}
				}
			}
			else {
				/* Failure to create a SSL handler */
				nSslError = ERR_get_error();
				nresult = sslError2nresult(nSslError, &strError);
				log_error(L_SOCKET, "[ssl_socket] can't create ssl handle, ssl error %d (%s)\n",
						  					nSslError, strError);
				if ( !bSslCtx )  {
					deleteSslContext();
				}
			}
		}
	}

	return nresult;
}

/*
 * Free SSL handle if any
 *
 * The SSL client or server that initiates the SSL closure calls SSL_shutdown()
 * either once or twice. If it calls the API twice, one call sends
 * the close_notify alert and one call receives the response from the peer.
 * If the initator calls the API only once, the initiator does not receive
 * the close_notify alert from the peer. (The initiator is not required
 * to wait for the responding alert).
 *
 * The peer that receives the alert calls SSL_shutdown() once to send
 * the alert to the initiating party.
 */
void CSslSocketAsync::deleteSslHandle()
{
	if ( m_pSslHandle != nullptr )  {
		SSL_shutdown(m_pSslHandle);
		SSL_shutdown(m_pSslHandle);
		SSL_free(m_pSslHandle);
		m_pSslHandle = nullptr;
	}
}

/*
 * [Public API function]
 *
 * Create a new SSL socket
 *
 * 		bindAddr		source address, optional, may be NETADDR_NULL
 * 		sockType		socket type, supported type SOCKET_TYPE_STREAM only
 *
 * Return:
 * 		ESUCCESS		connected
 *		EINTR			signal cause
 *		...
 */
result_t CSslSocketAsync::open(const CNetAddr& bindAddr, socket_type_t sockType)
{
	shell_assert(sockType == SOCKET_TYPE_STREAM);
	return CSocketAsync::open(bindAddr, sockType);
}

/*
 * Unsupported TLS/SSL API function
 *
 * Return: EINVAL
 */
result_t CSslSocketAsync::open(const char* strBindSocket, socket_type_t sockType)
{
	shell_unused(strBindSocket);
	shell_unused(sockType);

	return EINVAL;
}

/*
 * [Public API function]
 *
 * 		dstAddr			address to connect to, required
 * 		bindAddr		local address to bind to, optional, may be NETHOST_NULL
 * 		sockType		socket type, supported type SOCKET_TYPE_STREAM only
 *
 * Return:
 * 		ESUCCESS		connected
 * 		EINPROGRESS		connection is not yet connected, retry
 *		EINTR			interrupted by external signal
 *		...				fatal error
 */
result_t CSslSocketAsync::connectAsync(const CNetAddr& dstAddr, const CNetAddr& bindAddr,
							  			socket_type_t sockType)
{
	result_t 	nresult;

	m_bTcpConnected = false;

	if ( sockType != SOCKET_TYPE_STREAM )  {
		return EINVAL;
	}

	nresult = CSocketAsync::connectAsync(dstAddr, bindAddr, sockType);
	if ( nresult == ESUCCESS )  {
		m_bTcpConnected = true;

		nresult = connectSslAsync();
		if ( nresult != ESUCCESS && nresult != EINPROGRESS )  {
			CSocketAsync::close();
		}
	}

	return nresult;
}

/*
 * Wait for the TCP connection to the remote server and perform SSL handshake
 *
 * Return:
 * 		ESUCCESS		completed
 * 		EINPROGRESS		either TCP is not connected or SSL handshake is not completed
 * 		...				fatal error
 *
 * Note:
 * 		1) Must be called after CSslSocket::connectAsync()
 * 		2) Call can be repeated
 */
result_t CSslSocketAsync::connectSslAsync()
{
	result_t	nresult = EINVAL;

	if ( isOpen() )  {
		nresult = ESUCCESS;

		/*
		 * Check for TCP connection
		 */
		if ( !m_bTcpConnected )  {
			/* No TCP connection yet */
			struct pollfd	pfd[1];
			int				n;

			pfd[0].fd = m_hSocket;
			pfd[0].events = POLLIN|POLLOUT|POLLPRI;
			pfd[0].revents = 0;

			n = ::poll(pfd, 1, 0);
			if ( n == 0 )  {
				nresult = EINPROGRESS;		/* Not connected */
			} else if ( n > 0 )  {
				m_bTcpConnected = true;		/* Connected to the remote server */
			} else {
				nresult = errno;
			}
		}

		/*
		 * Check for SSL connection
		 */
		if ( nresult == ESUCCESS )  {
			int 			nSslResult, nSslError;
			const char*		strError;
			boolean_t 		bSslHandle = m_pSslHandle != nullptr;

			if ( m_pSslHandle == nullptr )  {
				nresult = createSslHandle();
			}

			if ( nresult == ESUCCESS )  {
				/* Start/check TLS/SSL handshake with a server */
				ERR_clear_error();
				nSslResult = SSL_connect(m_pSslHandle);
				if ( nSslResult == 1 )  {
					/* Handshake completed, connection established */
				}
				else {
					nSslError = SSL_get_error(m_pSslHandle, nSslResult);
					if ( nSslError == SSL_ERROR_WANT_READ || nSslError == SSL_ERROR_WANT_WRITE ||
							nSslError == SSL_ERROR_WANT_CONNECT || nSslError == SSL_ERROR_WANT_ACCEPT )  {
						nresult = EINPROGRESS;
					}
					else {
						nresult = sslError2nresult(nSslError, &strError);
						log_error(L_SOCKET, "[ssl_socket] can't create ssl connection, ssl error %d (%s)\n",
							  					nSslError, strError);
						if ( !bSslHandle ) {
							deleteSslHandle();
							deleteSslContext();
						}
					}
				}
			}
		}
	}

	return nresult;
}

/*
 * [Public API function]
 *
 * Close SSL connection and socket
 *
 * Return: ESUCCESS, ...
 */
result_t CSslSocketAsync::close()
{
	result_t	nresult;

	deleteSslHandle();
	deleteSslContext();

	nresult = CSocketAsync::close();
	return nresult;
}

/*
 * [Public API function]
 *
 * Send up to specified bytes asynchronous
 *
 * 		pBuffer			input buffer
 * 		pSize			IN: maximum bytes to send, OUT: sent bytes
 * 		destAddr		must be NETADDR_NULL
 *
 * Return:
 * 		ESUCCESS		all data sent
 * 		EAGAIN			some data sent
 * 		EBADF			socket is not connected
 * 		ECONNRESET		connection closed by remote peer
 * 		EINTR			interrupted by signal
 * 		...				fatal error
 */
result_t CSslSocketAsync::sendAsync(const void* pBuffer, size_t* pSize, const CNetAddr& destAddr)
{
	result_t 		nresult;
	int 			nSslResult, nSslError;
	const char*		strError;

	shell_assert(!destAddr.isValid());

	if ( !isOpen() || m_pSslHandle == nullptr )  {
		return EBADF;		/* Socket is not connected */
	}

	if ( *pSize == 0 )  {
		return ESUCCESS;
	}

	ERR_clear_error();
	nSslResult = SSL_write(m_pSslHandle, pBuffer, *pSize);
	if ( nSslResult > 0 )  {
		/* Written full buffer */
		nresult = ESUCCESS;
	}
	else {
		*pSize = 0;
		nSslError = SSL_get_error(m_pSslHandle, nSslResult);
		if ( nSslError == SSL_ERROR_WANT_READ || nSslError == SSL_ERROR_WANT_WRITE ||
				nSslError == SSL_ERROR_WANT_CONNECT || nSslError == SSL_ERROR_WANT_ACCEPT )  {
			nresult = EAGAIN;
		}
		else {
			nresult = sslError2nresult(nSslError, &strError);
			log_error(L_SOCKET, "[ssl_socket] ssl write failure, ssl error %d (%s)\n",
					  					nSslError, strError);
		}
	}

	return nresult;
}

/*
 * [Public API function]
 *
 * Receive up to specified bytes asynchronous
 *
 * 		pBuffer			output bytes
 * 		pSize			IN: maximum bytes to receive, OUT: received bytes
 *		pSrcAddr		must be nullptr
 *
 * Return:
 * 		ESUCCESS		all data has been received
 * 		EAGAIN			partial data has been received
 * 		EBADF			socket is not connected
 * 		ECONNRESET		connection closed by remote peer
 * 		EINTR			interrupted by signal
 * 		...
 */
result_t CSslSocketAsync::receiveAsync(void* pBuffer, size_t* pSize, CNetAddr* pSrcAddr)
{
	size_t			length, size;
	char*			p = (char*)pBuffer;
	result_t 		nresult;
	int 			nSslResult, nSslError;
	const char*		strError;

	shell_assert(pSrcAddr == nullptr);

	if ( !isOpen() || m_pSslHandle == nullptr )  {
		return EBADF;		/* Socket is not connected */
	}

	size = *pSize;
	length = 0;
	nresult = ESUCCESS;

	while ( length < size )  {
		ERR_clear_error();
		nSslResult = SSL_read(m_pSslHandle, p+length, size-length);
		if ( nSslResult > 0 )  {
			length += nSslResult;
		}
		else {
			nSslError = SSL_get_error(m_pSslHandle, nSslResult);
			if ( nSslError == SSL_ERROR_WANT_READ || nSslError == SSL_ERROR_WANT_WRITE ||
					nSslError == SSL_ERROR_WANT_CONNECT || nSslError == SSL_ERROR_WANT_ACCEPT )  {
				nresult = EAGAIN;
			}
			else {
				nresult = sslError2nresult(nSslError, &strError);
				log_error(L_SOCKET, "[ssl_socket] ssl read failure, ssl error %d (%s)\n",
						  					nSslError, strError);
			}
			break;
		}
	}

	*pSize = length;
	return nresult;
}

/*
 * [Public API function]
 *
 * Receive line
 *
 * 		pBuffer			output buffer
 * 		nPreSize		filled bytes of the output buffer
 * 		pSize			IN: maximum bytes to receive, OUT: received bytes
 * 		strEol			eol marker
 *
 * Return:
 * 		ESUCCESS		successfully received (maximum bytes or up to eol)
 * 		EAGAIN			partial data has been received
 * 		EBADF			socket is not connected
 * 		ECONNRESET		connection closed by remote peer
 * 		EINTR			interrupted by signal
 * 		...				fatal error
 */
result_t CSslSocketAsync::receiveLineAsync(void* pBuffer, size_t nPreSize, size_t* pSize,
												const char* strEol)
{
	size_t				size, length, lenEol, size2;
	char*				p = (char*)pBuffer + nPreSize;
	result_t			nresult;

	if ( !isOpen() || m_pSslHandle == nullptr )  {
		return EBADF;		/* Socket is not connected */
	}

	lenEol = _tstrlen(strEol);
	size = *pSize;
	length = 0;
	nresult = ESUCCESS;

	//log_trace(L_SOCKET, "[ssl_socket] receiving, presize %d, size %d\n", nPreSize, size);

	while ( length < size ) {
		size2 = 1;
		nresult = CSslSocketAsync::receiveAsync(p+length, &size2);
		if ( nresult != ESUCCESS )  {
			break;
		}

		if ( size2 != 1 )  {
			log_error(L_SOCKET, "[ssl_socket] unexpected read error: read 1 byte but got %d bytes\n", size2);
			nresult = EINVAL;
			break;
		}

		++length;
		if ( (length+nPreSize) >= lenEol )  {
			/* Check for eol */
			if ( _tmemcmp((char*)pBuffer+nPreSize+length-lenEol, strEol, lenEol) == 0 )  {
				//log_trace(L_SOCKET, "[ssl_socket] detected EOL\n");
				break;	/* Done */
			}
		}
	}

	*pSize = length;
	return nresult;
}

/*******************************************************************************
 * Synchronous TLS/SSL socket class
 */

/*
 * Construct a socket object
 *
 * 		bClient			true: use socket in client mode, false: use socket in server mode
 * 		exOption		additional options (see CSocket)
 */
CSslSocket::CSslSocket(boolean_t bClient, int exOption) :
	CSslSocketAsync(bClient, exOption)
{
}

CSslSocket::~CSslSocket()
{
}

/*
 * [Public API function]
 *
 * Connect to the destination IPv4 server
 *
 * 		dstAddr			destination address to connect to, required
 * 		hrTimeout		connect awaiting maximum timeout, required
 * 		bindAddr		local address to bind to, optional, may be NETADDR_NULL
 * 		sockType		socket type, supported SOCKET_TYPE_STREAM only
 *
 * Return:
 * 		ESUCCESS		connected
 * 		TIMEDOUT		timeout on connecting
 * 		EINTR			interrupted
 * 		ECANCELED		manual cancel
 * 		...				fatal error
 */
result_t CSslSocket::connect(const CNetAddr& dstAddr, hr_time_t hrTimeout,
						 	const CNetAddr& bindAddr, socket_type_t sockType)
{
	hr_time_t		hrStart, hrElapsed, hrTimeout1;
	short			revents;
	result_t 		nresult;

	if ( sockType != SOCKET_TYPE_STREAM )  {
		return EINVAL;
	}

	hrStart = hr_time_now();

	nresult = CSslSocketAsync::connectAsync(dstAddr, bindAddr);
	if ( nresult != EINPROGRESS )  {
		return nresult;
	}

	nresult = ETIMEDOUT;
	while ( (hrElapsed=hr_time_get_elapsed(hrStart)) < hrTimeout ) {
		/*
		 * Awaiting for a connection
		 */
		hrTimeout1 = hrTimeout-hrElapsed;

		nresult = CSocket::select(hrTimeout1, pollRead|pollWrite, &revents);
		if ( nresult != ESUCCESS ) {
			break;
		}

		nresult = checkREvents(revents);
		nresult = nresult == ECONNRESET ? ECONNREFUSED : nresult;
		if ( nresult != ESUCCESS )  {
			log_trace(L_SOCKET, "[socket_ssl] failed to connect, revents 0x%x, result: %d\n",
						  				revents, nresult);
			break;
		}

		nresult = CSslSocketAsync::connectSslAsync();
		if ( nresult != EINPROGRESS )  {
			break;
		}

		nresult = ETIMEDOUT;
	}

	if ( nresult != ESUCCESS )  {
		CSslSocketAsync::close();
	}

	return nresult;
}

/*
 * [Public API function]
 *
 * Unsupported TLS/SSL API function
 */
result_t CSslSocket::connect(const char* strDstSocket, hr_time_t hrTimeout,
							const char* strBindSocket, socket_type_t sockType)
{
	return EINVAL;
}

/*
 * [Public API function]
 *
 * Send data to the socket
 *
 * 		pBuffer			input buffer
 * 		pSize			IN: bytes to send, OUT: sent bytes
 * 		hrTimeout		maximum send time, required
 * 		destAddr		must be NETADDR_NULL
 *
 * Return:
 * 		ESUCCESS		all data sent
 * 		ETIMEDOUT		no data has been received within hrTimeout time
 * 		EBADF			socket is not connected
 * 		ECONNRESET		connection closed by remote peer
 * 		EINTR			interrupted by signal
 * 		ECANCELED		manual cancel
 * 		...				fatal error
 */
result_t CSslSocket::send(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout,
					  		const CNetAddr& destAddr)
{
	size_t			size;
	hr_time_t		hrStart, hrElapsed, hrTimeout1;
	short			revents;
	result_t		nresult;

	nresult = ETIMEDOUT;
	hrStart = hr_time_now();

	while ( (hrElapsed=hr_time_get_elapsed(hrStart)) < hrTimeout ) {
		size = *pSize;
		nresult = CSslSocketAsync::sendAsync(pBuffer, &size);
		if ( nresult != EAGAIN )  {
			break;
		}

		hrTimeout1 = hrTimeout-hrElapsed;

		nresult = CSocket::select(hrTimeout1, pollRead|pollWrite, &revents);
		if ( nresult != ESUCCESS ) {
			break;
		}

		nresult = checkREvents(revents);
		nresult = nresult == ECONNRESET ? ECONNREFUSED : nresult;
		if ( nresult != ESUCCESS )  {
			log_trace(L_SOCKET, "[socket_ssl] failed to send, revents 0x%x, result: %d\n",
					  				revents, nresult);
			break;
		}

		nresult = ETIMEDOUT;
	}

	*pSize = size;
	return nresult;
}

/*
 * [Public API function]
 *
 * Receive a data
 *
 * 		pBuffer			output buffer
 * 		pSize			IN: maximum bytes to receive, OUT: received bytes
 * 		options			receive options (CSocket::readFull)
 * 		hrTimeout		maximum receive time
 * 		pSrcAddr		must be nullptr
 *
 * Return:
 * 		ESUCCESS		some data has been received
 * 		ETIMEDOUT		no data has been received within hrTimeout time
 * 		EINTR			interrupted by signal
 * 		ECANCELED		manual cancel
 * 		...				fatal error
 */
result_t CSslSocket::receive(void* pBuffer, size_t* pSize, int options,
						  		hr_time_t hrTimeout, CNetAddr* pSrcAddr)
{
	uint8_t*		pBuf = (uint8_t*)pBuffer;
	size_t			size, length, size1;
	result_t		nresult;
	hr_time_t		hrStart, hrElapsed, hrTimeout1;
	short			revents;

	shell_assert(pSrcAddr == nullptr);

	size = *pSize;
	length = 0;

	nresult = ETIMEDOUT;
	hrStart = hr_time_now();

	while ( (hrElapsed=hr_time_get_elapsed(hrStart)) < hrTimeout ) {
		if ( length >= size )  {
			nresult = ESUCCESS;
			break;
		}

		size1 = size-length;
		nresult = CSslSocketAsync::receiveAsync(pBuf+length, &size1);
		if ( nresult == ESUCCESS )  {
			length += size1;
			continue;
		}

		if ( nresult != EAGAIN )  {
			break;
		}

		length += size1;
		if ( length > 0 && (options&CSocket::readFull) == 0 )  {
			nresult = ESUCCESS;
			break;
		}

		/*
		 * Awaiting data
		 */
		hrTimeout1 = hrTimeout-hrElapsed;

		nresult = CSocket::select(hrTimeout1, pollRead|pollWrite, &revents);
		if ( nresult != ESUCCESS ) {
			break;
		}

		nresult = checkREvents(revents);
		nresult = nresult == ECONNRESET ? ECONNREFUSED : nresult;
		if ( nresult != ESUCCESS )  {
			log_trace(L_SOCKET, "[socket_ssl] failed to read, revents 0x%x, result: %d\n",
					  				revents, nresult);
			break;
		}

		nresult = ETIMEDOUT;
	}

	*pSize = length;
	return nresult;
}

/*
 * Receive line
 *
 * 		pBuffer			output buffer
 * 		pSize			IN: maximum bytes to receive,
 * 						OUT: received bytes
 * 		strEol			eol marker
 * 		hrTimeout		maximum timeout (HR_0: receive forever)
 *
 * Return:
 * 		ESUCCESS		successfully received (maximum bytes or up to eol)
 * 		ETIMEDOUT		no data has been received within hrTimeout time
 * 		EBADF			socket is not connected
 * 		EINTR			interrupted by signal
 * 		ECANCELED		manual cancel
 * 		...
 *
 * Note: The result string contains trailing '\0'
 */
result_t CSslSocket::receiveLine(void* pBuffer, size_t* pSize, const char* strEol,
							  		hr_time_t hrTimeout)
{
	uint8_t*		pBuf = (uint8_t*)pBuffer;
	size_t			size, length, size1, szPre, lenEol;
	result_t		nresult;
	hr_time_t		hrStart, hrElapsed, hrTimeout1;
	short			revents;

	size = *pSize;

	if ( size < 2 )  {
		if ( size > 0 )  {
			*((char*)pBuffer) = '\0';
		}
		*pSize = 0;
		return ESUCCESS;
	}

	--size;
	length = 0;
	lenEol = _tstrlen(strEol);
	nresult = ETIMEDOUT;
	hrStart = hr_time_now();

	while ( (hrElapsed=hr_time_get_elapsed(hrStart)) < hrTimeout ) {
		if ( length >= size )  {
			nresult = ESUCCESS;
			break;
		}

		szPre = sh_min(length, lenEol);
		size1 = size - length;
		nresult = CSslSocketAsync::receiveLineAsync(pBuf+length-szPre, szPre, &size1, strEol);
		if ( nresult == ESUCCESS )  {
			length += size1;
			break;
		}

		if ( nresult != EAGAIN )  {
			break;
		}

		/*
		 * Awaiting data
		 */
		hrTimeout1 = hrTimeout-hrElapsed;

		nresult = CSocket::select(hrTimeout1, pollRead|pollWrite, &revents);
		if ( nresult != ESUCCESS ) {
			break;
		}

		nresult = checkREvents(revents);
		nresult = nresult == ECONNRESET ? ECONNREFUSED : nresult;
		if ( nresult != ESUCCESS )  {
			log_trace(L_SOCKET, "[socket_ssl] failed to read, revents 0x%x, result: %d\n",
					  				revents, nresult);
			break;
		}

		nresult = ETIMEDOUT;
	}

	((char*)pBuffer)[length] = '\0';
	*pSize = length;
	return nresult;
}
