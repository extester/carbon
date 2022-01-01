/*
 *  Shell library
 *  TCP and UNIX sockets
 *
 *  Copyright (c) 2013-2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 30.01.2015 18:34:23
 *      Initial revision.
 *
 *  Revision 1.1, 04.04.2015 23:22:12
 *  	Added various connect() functions;
 *  	Added readCorrect() function (to be renamed to read()).
 *
 *	Revision 1.2, 21.07.2016 15:12:25
 *		Added CFileBreaker object.
 *
 *  Revision 2.0, 23.10.2017 16:16:14
 *  	Changed closed socket handle from 0 to -1,
 *  	revised CAsyncSocket and CSocket open/connect functions.
 *
 *	Revision 2.1, 07.03.2018 17:17:42
 *		Enabled and reordered option 'reuse port' in CSocket::open().
 *		(Inserted before bind())
 */
/*
 * End of line:
 *
 * 		Carriage Return:	CR, 0x0D, 13 (decimal), "\r"
 * 		Line Feed:			LF, 0x0A, 10 (decimal), "\n"
 *
 * 	End-Of-Line variants:
 * 		CRLF
 * 		CR
 * 		LF
 */

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/utils.h"
#include "shell/socket.h"

#ifndef SO_REUSEPORT
#define SO_REUSEPORT 		15
#endif /* SO_REUSEPORT */

#define ASSERT_SOCKET_TYPE(__type)		\
	shell_assert( (((__type)&0xfff) == SOCKET_TYPE_STREAM) || \
					(((__type)&0xfff) == SOCKET_TYPE_UDP) || \
					(((__type)&0xfff) == SOCKET_TYPE_RAW) )

/*******************************************************************************
 * Asynchronous socket I/O class
 */

/*
 * Set a socket option
 *
 * 		fd				open socket descriptor
 * 		option			option to set, SO_xxx
 * 		value			option value
 *
 * Return: ESUCCESS, ...
 */
result_t CSocketAsync::setOption(int fd, int option, int value)
{
	int			retVal;
	result_t	nresult = ESUCCESS;

	retVal = ::setsockopt(fd, SOL_SOCKET, option, (const void*)&value, sizeof(value));
	if ( retVal < 0 )  {
		nresult = errno;
		log_error(L_SOCKET, "[socket] can't set socket option %d, value %d, result: %s(%d)\n",
					option, value, strerror(nresult), nresult);
	}

    return nresult;
}

/*
 * Set a socket option
 *
 * 		option			option to set, SO_xxx
 * 		pValue			option value buffer
 * 		valueLen		buffer length
 *
 * Return:
 * 		ESUCCESS		option set successful
 * 		EBADF			socket is not open
 * 		...				other error
 */
result_t CSocketAsync::setOption(int option, const void* pValue, int valueLen)
{
	int			retVal;
	result_t	nresult = ESUCCESS;

	if ( !isOpen() )  {
		return EBADF;
	}

	retVal = ::setsockopt(m_hSocket, SOL_SOCKET, option, pValue, (socklen_t)valueLen);
	if ( retVal < 0 )  {
		nresult = errno;
		log_error(L_SOCKET, "[socket] can't set socket option %d, result: %s(%d)\n",
				  option, strerror(nresult), nresult);
	}

	return nresult;
}

/*
 * Get a socket option
 *
 * 		option			option to get, SO_xxx
 * 		pValue			option value buffer
 * 		valueLen		buffer length
 *
 * Return:
 * 		ESUCCESS		option get successful
 * 		EBADF			socket is not open
 * 		...				other error
 */
result_t CSocketAsync::getOption(int option, void* pValue, int* pValueLength) const
{
	int			retVal;
	socklen_t	sockLen = (socklen_t)*pValueLength;
	result_t	nresult = ESUCCESS;

	if ( !isOpen() )  {
		return EBADF;
	}

	retVal = ::getsockopt(m_hSocket, SOL_SOCKET, option, pValue, &sockLen);
	*pValueLength = sockLen;
	if ( retVal < 0 )  {
		nresult = errno;
		log_error(L_SOCKET, "[socket] can't get socket option %d, result: %d\n",
				  option, nresult);
	}

	return nresult;
}

/*
 * Get a last socket error and clears error status on the socket
 *
 * 		pError			last error code [out]
 *
 * Return:
 * 		ESUCCESS		error code obtained successful (may be 0)
 * 		EBADF			socket is not open
 * 		...				other error
 */
result_t CSocketAsync::getError(int* pError) const
{
	return getOption(SO_ERROR, pError);
}

/*
 * Get the current IP/Port to which the socket is bound
 *
 * Return: ip/port
 */
CNetAddr CSocketAsync::getAddr() const
{
	struct sockaddr_in	sin;
	socklen_t 			len = sizeof(sin);
	int 				retVal;
	CNetAddr			netAddr;

	if ( isOpen() )  {
		retVal = ::getsockname(m_hSocket, (struct sockaddr *)&sin, &len);
		if ( retVal >= 0 )  {
			netAddr = CNetAddr(sin.sin_addr, ntohs(sin.sin_port));
		}
	}

	return netAddr;
}

/*
 * Open a IPv4 socket
 *
 * 		bindAddr		source address, optional, may be NETADDR_NULL
 * 		sockType		socket type, SOCKET_TYPE_xxx
 *
 * Return:
 * 		ESUCCESS		connected
 *		EINTR			signal cause
 *		...
 *
 * Examples:
 * 	Open IPv4/TCP socket:	open(bindAddr);
 * 	Open IPv4/UDP socket:	open(bindAddr, SOCKET_TYPE_UDP);
 * 	Open IPv4/ICMP socket:	open(bindAddr, SOCKET_TYPE_RAW);
 */
result_t CSocketAsync::open(const CNetAddr& bindAddr, socket_type_t sockType)
{
	int			fd, on, retVal, proto;
	result_t	nresult;

	if ( isOpen() )  {
		log_warning(L_SOCKET, "[socket] socket was open! Closed...\n");
		close();
	}

	/*
	 * Create socket of specified type
	 */
	ASSERT_SOCKET_TYPE(sockType);

	proto = (sockType&(~(SOCKET_TYPE_CLOEXEC|SOCKET_TYPE_NONBLOCK))) == SOCKET_TYPE_RAW
				? IPPROTO_ICMP : 0;

	fd = ::socket(AF_INET, SOCKET_TYPE_NONBLOCK|sockType, proto);
	if ( fd < 0 )  {
		nresult = errno;
		log_error(L_SOCKET, "[socket] failed to create socket type %Xh, result: %d (%s)\n",
				  sockType, nresult, strerror(nresult));
		return nresult;
	}

	/*
	 * Setup various options
	 */
	if ( !(m_exOption&CSocket::noReuseAddr) )  {
		setOption(fd, SO_REUSEADDR, 1);
	}

	if ( !(m_exOption&CSocket::noReusePort) )  {
		setOption(fd, SO_REUSEPORT, 1);
	}

	if ( m_exOption&CSocket::broadcast )  {
		/* Enable broadcast */
		setOption(fd, SO_BROADCAST, 1);
	}

	if ( proto == IPPROTO_ICMP )  {
		on = 1;
		retVal = ::setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
		if ( retVal < 0 ) {
			nresult = errno;
			::close(fd);
			log_error(L_SOCKET, "[socket] can't set IP_HRDINCL, socket %s, result: %d(%s)\n",
					  	bindAddr.cs(), nresult, strerror(nresult));
			return nresult;
		}
	}

	/*
	 * Bind socket source address
	 */
	if ( bindAddr.isValid() )  {
		struct sockaddr_in  sockaddr_tmp;

		log_trace(L_SOCKET, "[socket] binding socket to %s\n", bindAddr.cs());

		_tbzero_object(sockaddr_tmp);
		sockaddr_tmp.sin_family = AF_INET;
		sockaddr_tmp.sin_addr = bindAddr;
		sockaddr_tmp.sin_port = htons((ip_port_t)bindAddr);

		retVal = ::bind(fd, (struct sockaddr*)&sockaddr_tmp, sizeof(sockaddr_tmp));
		if ( retVal < 0 ) {
			nresult = errno;
			log_error(L_SOCKET, "[socket] can't bind socket to %s, result: %d (%s)\n",
					  bindAddr.cs(), nresult, strerror(nresult));
			::close(fd);
			return nresult;
		}
	}

	m_hSocket = fd;
	return ESUCCESS;
}

/*
 * Open an UNIX socket
 *
 * 		strBindSocket	socket full filename, optional, may be NULL
 * 		sockType		socket type, SOCKET_TYPE_xxx
 *
 * Return:
 * 		ESUCCESS		connected
 *		EINTR			interrupted
 *		...
 */
result_t CSocketAsync::open(const char* strBindSocket, socket_type_t sockType)
{
	int         fd, retVal;
	result_t	nresult;

	/*
	 * Sanity checks
	 */
	ASSERT_SOCKET_TYPE(sockType);

	if ( isOpen() )  {
		log_warning(L_SOCKET, "[socket] socket was open, close!\n");
		close();
	}

    /*
     * Always create a non-blocking socket
     */
	fd = ::socket(AF_UNIX, SOCK_NONBLOCK|sockType, 0);
	if ( fd < 0 )  {
		nresult = errno;
		log_error(L_SOCKET, "[socket] failed to create socket, result %d\n", nresult);
		return nresult;
	}

    /*
     * Bind socket descriptor to the file system socket
     */
	nresult = ESUCCESS;

	if ( strBindSocket )  {
		struct sockaddr_un 	sockaddr;

		_tbzero(&sockaddr, sizeof(sockaddr));
		sockaddr.sun_family = AF_UNIX;
		strncpy(sockaddr.sun_path, strBindSocket, sizeof(sockaddr.sun_path) - 1);

		retVal = ::bind(fd, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
		if ( retVal >= 0 ) {
			m_hSocket = fd;
			nresult = ESUCCESS;
		}
		else {
			nresult = errno;
			log_error(L_SOCKET, "[socket] error on binding socket '%s', result: %d\n",
									strBindSocket, nresult);
			::close(fd);
		}
	}
	else {
		m_hSocket = fd;
	}

	return nresult;
}

/*
 * Connect to the remote peer
 *
 * 		dstAddr			address to connect to, required
 * 		bindAddr		local address to bind to, optional, may be NETHOST_NULL
 * 		sockType		socket type, SOCKET_TYPE_xxx
 *
 * Return: ESUCCESS, EINPROGRESS, ...
 */
result_t CSocketAsync::connectAsync(const CNetAddr& dstAddr, const CNetAddr& bindAddr,
							   socket_type_t sockType)
{
	struct sockaddr_in  sockaddr;
	int					retVal, on;
	result_t			nresult;

    /*
     * Create a new socket
     */
	nresult = open(bindAddr, sockType);
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    /*
     * Set socket 'NODELAY' option
     */
    on = 1;
    retVal = ::setsockopt(m_hSocket, IPPROTO_TCP, TCP_NODELAY, (const void*)&on, sizeof(on));
    if ( retVal < 0 )  {
    	log_warning(L_SOCKET, "[socket] can't set socket NODELAY, connect to %s, result: %d\n",
							dstAddr.cs(), errno);
    }

    /*
     * Start connection
     */
    _tbzero(&sockaddr, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr = dstAddr;
    sockaddr.sin_port = htons((ip_port_t)dstAddr);

    retVal = ::connect(m_hSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if ( retVal == 0 )  {
        /* Socket is connected */
        return ESUCCESS;
    }

    if ( errno != EINPROGRESS )  {
        nresult = errno;
    	log_trace(L_SOCKET, "[socket] failed to connect to %s, result: %d\n",
							dstAddr.cs(), nresult);
        close();
        return nresult;
    }

    return EINPROGRESS;
}

/*
 * Connect to the remote peer
 *
 * 		strDstSocket	full filename of the socket to connect to, required
 * 		strBindSocket	full filename of the socket to bind to, optional, may be NULL
 * 		sockType		destination socket type, SOCKET_TYPE_xxx
 *
 * Return: ESUCCESS, EINPROGRESS, ...
 */
result_t CSocketAsync::connectAsync(const char* strDstSocket, const char* strBindSocket,
							   	socket_type_t sockType)
{
	struct sockaddr_un 	sockaddr;
	int              	retVal;
	result_t			nresult;

	shell_assert(strDstSocket);

	if ( isOpen() )  {
		log_warning(L_SOCKET, "[socket] socket was open, close!\n");
		close();
	}

	nresult = open(strBindSocket, sockType);
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	_tbzero(&sockaddr, sizeof(sockaddr));
	sockaddr.sun_family = AF_UNIX;
	copyString(sockaddr.sun_path, strDstSocket, sizeof(sockaddr.sun_path));

	retVal = ::connect(m_hSocket, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
	if ( retVal == 0 )  {
		/* Socket is connected */
		return ESUCCESS;
	}

	nresult = errno;
	if ( nresult != EINPROGRESS )  {
		log_trace(L_SOCKET, "[socket] failed to connect to %s, result: %d\n",
				  strDstSocket, nresult);
		close();
	}

	return nresult;
}

/*
 * Close socket if it was open
 *
 * Return: ESUCCESS, ...
 */
result_t CSocketAsync::close()
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	if ( m_hSocket >= 0 )  {
		retVal = ::close(m_hSocket);
		if ( retVal < 0 ) {
			nresult = errno;
			log_error(L_SOCKET, "[socket] failed to close socket, result: %d\n", nresult);
		}
		m_hSocket = -1;
	}

	return nresult;
}

/*
 * Send up to specified bytes asynchronous
 *
 * 		pBuffer			input buffer
 * 		pSize			IN: maximum bytes to send,
 * 						OUT: sent bytes
 *		destAddr		destination address (for non connection-mode sockets) (optional)
 *
 * Return:
 * 		ESUCCESS		all data sent
 * 		EAGAIN			some data sent
 * 		EBADF			socket is not connected
 * 		ECONNRESET		can't send data
 * 		EINTR			interrupted by signal
 * 		...
 */
result_t CSocketAsync::sendAsync(const void* pBuffer, size_t* pSize, const CNetAddr& destAddr)
{
	struct sockaddr_in	sockaddr;
	struct sockaddr*	psockaddr = NULL;
	socklen_t 			sockaddrlen = 0;
	size_t				length, size, l;
	ssize_t				len;
	result_t			nresult;

	if ( !isOpen() )  {
		*pSize = 0;
		return EBADF;
	}

	if ( destAddr.isValid() )  {
		int 		retVal;
		sa_family_t	domain;
		socklen_t	dlen;

		dlen = sizeof(domain);
		retVal = ::getsockopt(m_hSocket, SOL_SOCKET, SO_DOMAIN, &domain, &dlen);
		if ( retVal != 0 ) {
			nresult = errno;
			log_debug(L_SOCKET, "[socket] failed to get socket protocol, result: %d\n", nresult);
			*pSize = 0;
			return nresult;
		}

		_tbzero_object(sockaddr);
		sockaddr.sin_family = domain;
		sockaddr.sin_addr.s_addr = destAddr;
		sockaddr.sin_port = htons((ip_port_t)destAddr);

		psockaddr = (struct sockaddr*)&sockaddr;
		sockaddrlen = sizeof(sockaddr);
	}

	size = *pSize;
	length = 0;
	nresult = ESUCCESS;

	while( length < size )  {
		l = size-length;
		len = ::sendto(m_hSocket, (const char*)pBuffer+length, l, MSG_NOSIGNAL,
					   psockaddr, sockaddrlen);
		if ( len < 0 )  {
			nresult = errno;
			if ( errno != EAGAIN )  {
				log_debug(L_SOCKET, "[socket] send %d bytes failed, result: %s(%d)\n",
						  l, strerror(nresult), nresult);
			}
			break;
		}

		if ( len == 0 )  {
			log_debug(L_SOCKET, "[socket] send %d bytes returns ZERO, set ECONNRESET error\n", l);
			nresult = ECONNRESET;
			break;
		}

		length += len;
	}

	*pSize = length;
	return nresult;
}

/*
 * Receive up to specified bytes asynchronous
 *
 * 		pBuffer			output bytes
 * 		pSize			IN: maximum bytes to receive,
 * 						OUT: received bytes
 *		pSrcAddr		source address (for non connection-mode sockets, maybe NULL) [OUT]
 *
 * Return:
 * 		ESUCCESS		all data has been received
 * 		EAGAIN			partial data has been received
 * 		EBADF			socket is not connected
 * 		ECONNRESET		connection closed by remote peer
 * 		EINTR			interrupted by signal
 * 		...
 */
result_t CSocketAsync::receiveAsync(void* pBuffer, size_t* pSize, CNetAddr* pSrcAddr)
{
	struct sockaddr_in	sockaddr;
	socklen_t 			sockaddrlen;
	size_t				length, size, l;
	ssize_t				len;
	char*				p = (char*)pBuffer;
	result_t			nresult;

	//log_debug(L_GEN, "[socket] socket receiving %d bytes, pSocket %llXh\n", *pSize, pSrcAddr);

	if ( !isOpen() )  {
		*pSize = 0;
		return EBADF;
	}

	size = *pSize;
	length = 0;
	_tbzero_object(sockaddr);
	nresult = ESUCCESS;

	while( length < size )  {
		l = size-length;
		sockaddrlen = sizeof(sockaddr);
		len = ::recvfrom(m_hSocket, p+length, l, MSG_NOSIGNAL,
						 (struct sockaddr*)&sockaddr, &sockaddrlen);

		if ( len < 0 )  {
			nresult = errno;
			if ( nresult != EAGAIN )  {
				/* Read error */
				log_debug(L_SOCKET, "[socket] receive failed, result: %d\n", nresult);
			}
			break;
		}

		if ( len == 0 )  {
			/* Server closes the socket */
			log_trace(L_SOCKET, "[socket] connection closed by peer\n");
			nresult = ECONNRESET;
			break;
		}

		//log_debug(L_GEN, "[socket] received a byte '%d'\n", *(p+length));

		if ( length == 0 && pSrcAddr != NULL )  {
			/* Non connecion socket */
			*pSrcAddr = CNetAddr(sockaddr.sin_addr, ntohs(sockaddr.sin_port));
			length += len;
			break;
		}

		length += len;
	}

	*pSize = length;

	//log_debug(L_GEN, "socket receivED %d bytes\n", *pSize);
	//log_debug_bin(L_GEN, pBuffer, *pSize, "data: ");
	return nresult;
}

/*
 * Receive line
 *
 * 		pBuffer			output buffer
 * 		nPreSize		filled bytes of the output buffer
 * 		pSize			IN: maximum bytes to receive,
 * 						OUT: received bytes
 * 		strEol			eol marker
 *
 * Return:
 * 		ESUCCESS		successfully received (maximum bytes or up to eol)
 * 		EAGAIN			partial data has been received
 * 		EBADF			socket is not connected
 * 		ECONNRESET		connection closed by remote peer
 * 		EINTR			interrupted by signal
 * 		...
 */
result_t CSocketAsync::receiveLineAsync(void* pBuffer, size_t nPreSize, size_t* pSize,
								   const char* strEol)
{
	size_t				size, length, lenEol;
	ssize_t				len;
	char*				p = (char*)pBuffer + nPreSize;
	result_t			nresult;

	if ( !isOpen() )  {
		*pSize = 0;
		return EBADF;
	}

	lenEol = _tstrlen(strEol);
	size = *pSize;
	length = 0;
	nresult = ESUCCESS;

	log_trace(L_SOCKET, "[socket] receiving, presize %d, size %d\n", nPreSize, *pSize);

	while ( length < size )  {
		len = ::recv(m_hSocket, p+length, 1, MSG_NOSIGNAL);

		if ( len < 0 )  {
			nresult = errno;
			if ( nresult != EAGAIN )  {
				/* Read error */
				log_debug(L_SOCKET, "[socket] receive failed, result: %d\n", nresult);
			}
			break;
		}

		if ( len == 0 )  {
			/* Server closes the socket */
			log_trace(L_SOCKET, "[socket] connection closed by peer\n");
			nresult = ECONNRESET;
			break;
		}

		/*log_debug(L_SOCKET_FL, "[socket] received a byte '%c'\n", *(p+length));*/

		length += len;
		if ( (length+nPreSize) >= lenEol )  {
			/* Check for eol */
			if ( _tmemcmp((char*)pBuffer+nPreSize+length-lenEol, strEol, lenEol) == 0 )  {
				log_trace(L_SOCKET, "[socket] detected EOL\n");
				break;	/* Done */
			}
		}
	}

	*pSize = length;
	return nresult;
}

/*******************************************************************************
 * TCP/IP V4 and UNIX sockets
 */

/*
 * Awaiting for the I/O activity on the socket
 *
 * 		hrTimeout		awaiting timeout
 * 		options			awaiting event, pollRead or pollWrite
 *
 * Return:
 * 		ESUCCESS		i/o detected
 * 		EBADF			socket is not connected
 * 		ETIMEDOUT		no i/o detected
 *		ECANCEL			manual cancel
 *		EINTR			terminated
 */
result_t CSocket::select(hr_time_t hrTimeout, int options, short* prevents)
{
    struct pollfd       pfd[2];
    hr_time_t           hrStart, hrElapsed;
    int                 msTimeout, n;
    short				chevents;
	nfds_t				nfds;
    result_t			nresult;

	if ( !isOpen() )  {
		return EBADF;
	}

	shell_assert((options&(pollRead|pollWrite)) != 0);
	if ( (options&(pollRead|pollWrite)) == 0 )  {
		log_debug(L_SOCKET, "[socket] no poll option specified\n");
		return EINVAL;
	}

    nresult = ETIMEDOUT;
    hrStart = hr_time_now();

    while ( (hrElapsed=hr_time_get_elapsed(hrStart)) < hrTimeout ) {
        msTimeout = (int)HR_TIME_TO_MILLISECONDS(hrTimeout-hrElapsed);
        if ( msTimeout < 1 )  {
            break;
        }

        pfd[0].fd = m_hSocket;
        pfd[0].events = 0;
        if ( options&CSocket::pollRead ) {
        	pfd[0].events |= POLLIN|POLLPRI;
        }
        if ( options&CSocket::pollWrite )  {
        	pfd[0].events |= POLLOUT;
        }
        pfd[0].revents = 0;

		nfds = 1;
        if ( m_breaker.isEnabled() )  {
        	pfd[1].fd = m_breaker.getRHandle();
        	pfd[1].events = POLLIN;
        	pfd[1].revents = 0;
			nfds++;
        }

        n = ::poll(pfd, nfds, msTimeout);
        if ( n == 0 )  {
        	/* Timeout and no sockets are ready */
            break;
        }

        if ( n < 0 )  {
            /* Polling failed */
            nresult = errno;
			if ( nresult != EINTR ) {
				log_debug(L_SOCKET, "[socket] polling failed, result: %d\n", nresult);
			}
            break;
        }

        if ( m_breaker.isEnabled() && (pfd[1].revents&(POLLIN|POLLPRI)) )  {
            /* Canceled */
        	//log_debug(L_NET_FL, "[socket] poll cancelled\n");
			m_breaker.reset();
            nresult = ECANCELED;
            break;
        }

    	/*
    	 * Check events
    	 */
    	chevents = 0;
        if ( options&CSocket::pollRead ) {
        	chevents |= POLLIN|POLLPRI;
        }
        if ( options&CSocket::pollWrite ) {
        	chevents |= POLLOUT;
        }

        if ( pfd[0].revents&chevents )  {
        	/* Success */
        	if ( prevents )  {
        		*prevents = pfd[0].revents;
        	}
        	nresult = ESUCCESS;
        	break;
        }
    }

    return nresult;
}

/*
 * Check returned events from the CSocket::select() and
 * return appropriate result code
 *
 * 		revents		returned events from the select()
 *
 * Return: ESUCCESS, ECONNRESET, EINVAL, EIO
 */
result_t CSocket::checkREvents(short revents) const
{
	result_t	nresult = ESUCCESS;

	if ( (revents&POLLHUP) != 0 )  {
		nresult = ECONNRESET;
	} else
	if ( (revents&POLLNVAL) != 0 )  {
		nresult = EINVAL;
	} else
	if ( (revents&POLLERR) != 0 )  {
		nresult = EIO;
	}

	return nresult;
}

/*
 * Connect to the destination IPv4 server
 *
 * 		dstAddr			destination address to connect to, required
 * 		hrTimeout		connect awaiting maximum timeout, required
 * 		bindAddr		local address to bind to, optional, may be NETADDR_NULL
 * 		sockType		socket type, SOCKET_TYPE_xxx
 *
 * Return: ESUCCESS, ...
 */
result_t CSocket::connect(const CNetAddr& dstAddr, hr_time_t hrTimeout,
						  const CNetAddr& bindAddr, socket_type_t sockType)
{
	result_t	nresult;

	nresult = connectAsync(dstAddr, bindAddr, sockType);
	if ( nresult != ESUCCESS )  {
		if ( nresult == EINPROGRESS )  {
			/*
			 * Awaiting for connect
			 */
			short	revents;

			nresult = select(hrTimeout, pollRead|pollWrite, &revents);
			if ( nresult == ESUCCESS )  {
				nresult = checkREvents(revents);
				nresult = nresult == ECONNRESET ? ECONNREFUSED : nresult;
				if ( nresult != ESUCCESS )  {
					log_trace(L_SOCKET, "[socket] failed to connect, revents 0x%x, result: %d\n",
											revents, nresult);
					close();
				}
			}
			else {
				close();
			}
		}
	}

	return nresult;
}

/*
 * Connect to the UNIX domain socket
 *
 * 		strDstSocket	socket's full filename, required
 * 		hrTimeout		connect awaiting maximum timeout, required
 * 		strBindSocket	local socket full filename to bind to, optional, may be NULL
 * 		sockType		socket type, SOCKET_TYPE_xxx
 *
 * Return: ESUCCESS, ...
 */
result_t CSocket::connect(const char* strDstSocket, hr_time_t hrTimeout,
						  	const char* strBindSocket, socket_type_t sockType)
{
	result_t	nresult;

	nresult = connectAsync(strDstSocket, strBindSocket, sockType);
	if ( nresult != ESUCCESS )  {
		if ( nresult == EINPROGRESS )  {
			/*
			 * Awaiting for connect
			 */
			short	revents;

			nresult = select(hrTimeout, pollRead|pollWrite, &revents);
			if ( nresult == ESUCCESS )  {
				nresult = checkREvents(revents);
				nresult = nresult == ECONNRESET ? ECONNREFUSED : nresult;
				if ( nresult != ESUCCESS )  {
					log_trace(L_SOCKET, "[socket] failed to connect, revents 0x%x, result: %d\n",
							  revents, nresult);
					close();
				}
			}
			else {
				close();
			}
		}
	}

	return nresult;
}

result_t CSocket::send(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout,
						const CNetAddr& destAddr)
{
	const uint8_t*	pBuf = (const uint8_t*)pBuffer;
	size_t			size, length;
	result_t		nresult;
	hr_time_t		hrStart;
	short			revents;

	if ( !isOpen() )  {
		return EBADF;
	}

	length = 0;
	hrStart = hr_time_now();
	nresult = (*pSize) ? EAGAIN : ESUCCESS;

	while ( *pSize > length && nresult == EAGAIN )  {
		nresult = select(hr_timeout(hrStart, hrTimeout), pollWrite, &revents);
		if ( nresult == ESUCCESS )  {
			size = (*pSize) - length;
			nresult = sendAsync(pBuf+length, &size, destAddr);
			length += size;
		}
	}

	*pSize = length;
	return nresult;
}

result_t CSocket::receive(void* pBuffer, size_t* pSize, int options,
						  hr_time_t hrTimeout, CNetAddr* pSrcAddr)
{
	uint8_t*		pBuf = (uint8_t*)pBuffer;
	size_t			size, length;
	result_t		nresult;
	hr_time_t		hrStart;
	short			revents;

	if ( !isOpen() )  {
		return EBADF;
	}

	length = 0;
	hrStart = hr_time_now();
	nresult = (*pSize) ? EAGAIN : ESUCCESS;

	while ( *pSize > length && nresult == EAGAIN )  {
		nresult = select(hr_timeout(hrStart, hrTimeout), pollRead, &revents);
		if ( nresult == ESUCCESS )  {
			size = (*pSize) - length;
			nresult = receiveAsync(pBuf+length, &size, pSrcAddr);
			length += size;

			if ( !(options&CSocket::readFull) )  {
				if ( nresult == EAGAIN )  {
					nresult = ESUCCESS;		/* Stop receiving */
				}
			}
		}
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
 * 		EBADF			socket is not connected
 * 		EINTR			interrupted by signal
 * 		ECANCELED		manual cancel
 * 		...
 *
 * Note: The result string contains trailing '\0'
 */
result_t CSocket::receiveLine(void* pBuffer, size_t* pSize, const char* strEol,
							  hr_time_t hrTimeout)
{
	uint8_t*		pBuf = (uint8_t*)pBuffer;
	size_t			lenEol, length, szPre, size, size1;
	result_t		nresult;
	hr_time_t		hrStart;
	short			revents;

	if ( !isOpen() )  {
		return EBADF;
	}

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
	hrStart = hr_time_now();
	nresult = EAGAIN;

	while ( size > length && nresult == EAGAIN )  {
		nresult = select(hr_timeout(hrStart, hrTimeout), pollRead, &revents);
		if ( nresult == ESUCCESS )  {
			szPre = sh_min(length, lenEol);
			size1 = size - length;
			nresult = receiveLineAsync(pBuf+length-szPre, szPre, &size1, strEol);
			length += size1;
		}
	}

	((char*)pBuffer)[length] = '\0';
	*pSize = length;
	return nresult;
}

/*
 * Start listening on the socket
 *
 * 		count		maximum client count, dropped on overflow
 *
 * Return:
 * 		ESUCCESS	success
 * 		EBADF		socket is not open
 */
result_t CSocket::listen(int count)
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	if ( !isOpen() )  {
		return EBADF;
	}

	retVal = ::listen(m_hSocket, count);
	if ( retVal < 0 )  {
		nresult = errno;
		log_error(L_SOCKET, "[socket] listen failed, result: %d\n", nresult);
	}

	return nresult;
}

/*
 * Extracts the first request from the request queue
 *
 * Return: CSocket object or NULL
 */
CSocket* CSocket::accept()
{
	CSocket*			pSocket = NULL;
    struct sockaddr_un	sockaddr;
    socklen_t 			len;
    int 				fd;

    if ( !isOpen() )  {
    	return NULL;
    }

    len = sizeof(sockaddr);
    fd = ::accept4(m_hSocket, (struct sockaddr *)&sockaddr, &len, SOCK_NONBLOCK);
    if ( fd >= 0 )  {
    	pSocket = new CSocket(m_exOption);
    	pSocket->setHandle(fd);
    }

    return pSocket;
}

/*******************************************************************************
 * CSocketRef class
 */

/*
 * Extracts the first request from the request queue
 *
 * Return: CSocket object or NULL
 */
CSocketRef* CSocketRef::accept()
{
	CSocketRef*			pSocket = NULL;
	struct sockaddr_un	sockaddr;
	socklen_t 			len;
	int 				fd;

	if ( !isOpen() )  {
		return NULL;
	}

	len = sizeof(sockaddr);
	fd = ::accept4(m_hSocket, (struct sockaddr *)&sockaddr, &len, SOCK_NONBLOCK);
	if ( fd >= 0 )  {
		pSocket = new CSocketRef(m_exOption);
		pSocket->setHandle(fd);
	}

	return pSocket;
}

/*******************************************************************************
 * Debugging support
 */

#if CARBON_DEBUG_DUMP

void CSocketRef::dump(const char* strPref) const
{
	log_dump(" * %sSocket: %s, %s\n", strPref, (const char*)getAddr(),
			 isOpen() ? "OPEN" : "closed");
}

#endif /* CARBON_DEBUG_DUMP */
