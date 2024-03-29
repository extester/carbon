/*
 *  Carbon framework
 *  Http protocol container
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 29.09.2016 16:42:07
 *      Initial revision.
 */

#include <stdexcept>

#include "shell/shell.h"
#include "shell/debug.h"
#include "shell/dec_ptr.h"

#include "carbon/utils.h"
#include "carbon/memory.h"
#include "carbon/logger.h"
#include "carbon/http_container.h"


/*******************************************************************************
 * CHttpContainer class
 */

CHttpContainer::CHttpContainer() :
	CNetContainer(),
	m_pBody(0),
	m_nCurSize(0),
	m_nSize(0)
{
}

CHttpContainer::CHttpContainer(const char* strStart) :
	CNetContainer(),
	m_pBody(0),
	m_nCurSize(0),
	m_nSize(0)
{
	setStartLine(strStart);
}

CHttpContainer::CHttpContainer(const char* strStart, const char* strHeader) :
	CNetContainer(),
	m_pBody(0),
	m_nCurSize(0),
	m_nSize(0)
{
	setStartLine(strStart);
	appendHeader(strHeader);
}

CHttpContainer::~CHttpContainer()
{
	clear();
}

void CHttpContainer::clear()
{
	SAFE_FREE(m_pBody);
	m_nSize = m_nCurSize = 0;
	m_strHeader.clear();
	m_strStart.clear();
	CNetContainer::clear();
}

/*
 * Setup starting line on HTTP request
 *
 * 		strStart		start line
 */
void CHttpContainer::setStartLine(const char* strStart)
{
	const char	*p, *e;

	if ( !strStart )  {
		m_strStart.clear();
		return;
	}

	p = strStart;
	SKIP_CHARS(p, " \t");

	e = p;
	SKIP_NON_CHARS(e, HTTP_EOL "\0");

	m_strStart = CString(p, A(e)-A(p));
}

/*
 * Parse HTTP result code from start line
 *
 * 		pCode		parsed code (may be nullptr) [out]
 *
 * Return:
 * 		ESUCCESS		success, pCode contains http code
 * 		ENOENT			empty start line
 * 		EINVAL			can't parse code, invalid format
 *
 * Line format:
 * 		HTTP/1.0 <code> <str>
 * 		HTTP/1.1 <code> <str>
 * 		HTTP/2 <code> <str>
 */
result_t CHttpContainer::getCode(int* pCode) const
{
	const char*		s = m_strStart;
	size_t			length;
	result_t		nresult;

	if ( m_strStart.isEmpty() )  {
		return ENOENT;
	}

	SKIP_CHARS(s, " \t");
	length = _tstrlen(s);

	nresult = ESUCCESS;
	if ( length > 8 && _tmemcmp(s, "HTTP/1.1", 8) == 0 )  {
		s += 8;
	} else if ( length > 6 && _tmemcmp(s, "HTTP/2", 6) == 0 ) {
		s += 6;
	} else if ( length > 8 && _tmemcmp(s, "HTTP/1.0", 8) == 0 ) {
		s += 8;
	} else {
		nresult = EINVAL;
	}

	if ( nresult == ESUCCESS )  {
		const char*		p;
		int				code;

		SKIP_CHARS(s, " \t");
		p = s;
		SKIP_NON_CHARS(s, " \t");

		nresult = CString(p, A(s)-A(p)).getNumber(code);
		if ( nresult == ESUCCESS )  {
			if ( pCode )  { *pCode = code; }
		}
	}

	return nresult;
}

void CHttpContainer::appendHeader(const char* strHeader)
{
	const char 	*p, *e;

	if ( !strHeader )  {
		return;
	}

	p = strHeader;
	while ( *p != '\0' )  {
		SKIP_CHARS(p, " \t");
		e = p;
		SKIP_NON_CHARS(e, HTTP_EOL "\0");
		if ( p != e )  {
			/* Parsed a single header */
			const char* e1 = e-1;
			while ( e1 > p && _tstrchr(" \t", *e1) != 0 ) e1--;
			e1++;

			if ( p != e1 ) {
				m_strHeader += CString(p, A(e1) - A(p));
				m_strHeader += HTTP_EOL;
			}
		}

		p = e;

		SKIP_CHARS(p, HTTP_EOL " \t");
	}
}

/*
 * Allocate body buffer at least a given size
 *
 * 		nSize		minimum buffer size
 *
 * None: raise exception on out of memory
 */
void CHttpContainer::makeBuffer(size_t nSize)
{
	if ( m_nSize < nSize )  {
		size_t		newSize;
		uint8_t*	pBuffer;

		if ( nSize < 64 )  {
			newSize = 64;
		} else if ( nSize < 1024 )  {
			newSize = ALIGN(nSize, 64);
		} else if ( nSize < 4096 )  {
			newSize = ALIGN(nSize, 512);
		} else {
			newSize = ALIGN(nSize, 1024);
		}

		pBuffer = (uint8_t*)memRealloc(m_pBody, newSize);
		if ( !pBuffer )  {
			throw std::bad_alloc();
		}

		m_pBody = pBuffer;
		m_nSize = newSize;
	}
}

/*
 * Add a data to request body
 *
 * 		pBuffer		data to add
 * 		size		data length, bytes
 */
void CHttpContainer::appendBody(const void* pBuffer, size_t size)
{
	shell_assert(pBuffer);

	if ( pBuffer && size > 0 ) {
		makeBuffer(m_nCurSize + size + 1);
		UNALIGNED_MEMCPY(&m_pBody[m_nCurSize], pBuffer, size);
		m_nCurSize += size;
		m_pBody[m_nCurSize] = '\0';
	}
}

/*
 * Find a header
 *
 * 		strHeader		header to find
 *
 * Return: offset in headers or -1 if not found
 */
int CHttpContainer::findHeader(const char* strHeader) const
{
	int 		offset = -1;
	const char	*s, *p;
	size_t		n, n1;
	int 		res;

	shell_assert(strHeader);

	s = m_strHeader;
	n = _tstrlen(strHeader);

	while ( *s != '\0' )  {
		res = _tstrncasecmp(s, strHeader, n);
		if ( res == 0 )  {
			p = s+n;
			SKIP_CHARS(p, " \t");
			if ( *p == ':' )  {
				offset = (int)(A(s)-A(m_strHeader.cs()));
				break;
			}
		}

		n1 = _tstrcspn(s, HTTP_EOL);
		s += n1 + HTTP_EOL_LEN;
	}

	return offset;
}

/*
 * Get a specified/all headers
 *
 * 		strValue		headers [out]
 * 		strHeader		header to get value or NULL
 *
 * Note:
 * 		if strHeader != NULL returns a specified header value
 * 		if strHeader == NULL return all headers as a single string
 */
boolean_t CHttpContainer::getHeader(CString& strValue, const char* strHeader) const
{
	const char*		s = m_strHeader;
	size_t			n, n1;
	int 			offset;
	boolean_t		bResult = FALSE;

	if ( !strHeader )  {
		strValue = m_strHeader;
		return TRUE;
	}

	strValue = "";
	if ( s != 0 )  {
		offset = findHeader(strHeader);
		if ( offset >= 0 ) {
			n = _tstrlen(strHeader);

			s += offset + n;
			SKIP_CHARS(s, " \t");
			if ( *s == ':' )  {
				s++;
				SKIP_CHARS(s, " \t");

				n1 = _tstrcspn(s, HTTP_EOL);
				strValue = CString(s, n1);
				bResult = TRUE;
			}
		}
	}

	return bResult;
}

/*
 * Delete a specified/all headers
 *
 * 		strHeader		header to delete (NULL - delete all headers)
 */
void CHttpContainer::deleteHeader(const char* strHeader)
{
	int 	offset;

	if ( !strHeader )  {
		m_strHeader.clear();
		return;
	}

	offset = findHeader(strHeader);
	if ( offset >= 0 )  {
		const char*	s = m_strHeader;
		size_t		n;

		n = _tstrcspn(s+offset, HTTP_EOL) + HTTP_EOL_LEN;
		m_strHeader = CString(s, (size_t)offset) + CString(s+offset+n, m_strHeader.size()-offset-n);
	}
}

CNetContainer* CHttpContainer::clone()
{
	dec_ptr<CHttpContainer>	pContainer = new CHttpContainer(m_strStart, m_strHeader);

	if ( m_nCurSize > 0 ) {
		pContainer->appendBody(m_pBody, m_nCurSize);
	}

	pContainer->reference();
	return (CNetContainer*)(CHttpContainer*)pContainer;
}

result_t CHttpContainer::serialise(char** ppBuffer, size_t* pSize)
{
	char*		pBuffer;
	size_t		fullSize, len;
	result_t	nresult = ESUCCESS;

	fullSize = m_strStart.size() + HTTP_EOL_LEN + m_strHeader.size() +
				HTTP_EOL_LEN + m_nCurSize;
	pBuffer = (char*)memAlloc(fullSize);
	if ( pBuffer != 0 )  {
		len = m_strStart.size();
		UNALIGNED_MEMCPY(pBuffer, m_strStart.cs(), len);

		UNALIGNED_MEMCPY(&pBuffer[len], HTTP_EOL, HTTP_EOL_LEN);
		len += HTTP_EOL_LEN;

		UNALIGNED_MEMCPY(&pBuffer[len], m_strHeader.cs(), m_strHeader.size());
		len += m_strHeader.size();

		UNALIGNED_MEMCPY(&pBuffer[len], HTTP_EOL, HTTP_EOL_LEN);
		len += HTTP_EOL_LEN;

		UNALIGNED_MEMCPY(&pBuffer[len], m_pBody, m_nCurSize);

		*ppBuffer = pBuffer;
		*pSize = fullSize;
	}
	else {
		log_error(L_GEN, "[http_cont] out of memory allocating %d bytes\n", fullSize);
		nresult = ENOMEM;
	}

	return nresult;
}

/*
 * Send a container
 *
 * 		socket			open socket object
 * 		hrTimeout		maximum send time
 * 		dstAddr			destination address (for sendto())
 * 		pOffset			send offset [in/out]
 *
 * 	Return:
 * 		ESUCCESS	all data have been sent
 * 		EINTR		sending was interrupted by signal
 * 		...
 */
result_t CHttpContainer::send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr)
{
	char*		pBuffer;
	size_t		size;
	CString		strContentLength;
	int 		contentLength;
	result_t	nresult;

	if ( m_strStart.isEmpty() )  {
		log_error(L_GEN, "[http_cont] can't send container, starting line is not set\n");
		return EINVAL;
	}

	getHeader(strContentLength, HTTP_CONTENT_LENGTH);
	if ( strContentLength.getNumber(contentLength) != ESUCCESS || contentLength != (int)m_nCurSize )  {
		deleteHeader(HTTP_CONTENT_LENGTH);
		if ( m_nCurSize > 0 )  {
			char 	strBuf[64];

			_tsnprintf(strBuf, sizeof(strBuf), "%s: %u", HTTP_CONTENT_LENGTH, (unsigned int)m_nCurSize);
			appendHeader(strBuf);
		}
	}

	nresult = serialise(&pBuffer, &size);
	if ( nresult == ESUCCESS )  {
		//char *sss = (char*)memAlloc(size+1);
		//UNALIGNED_MEMCPY(sss, pBuffer, size); sss[size] = 0;
		//log_debug(L_GEN, "SERIALISE: '%s'\n", sss);

		nresult = socket.send(pBuffer, &size, hrTimeout, dstAddr);
		if ( nresult != ESUCCESS )  {
			char 		strTmp[128];
			CNetAddr	dst;

			getDump(strTmp, sizeof(strTmp));
			dst = (dstAddr == NETADDR_NULL) ? socket.getAddr() : dstAddr;

			log_debug(L_GEN, "[http_cont] failed to send container to %s, result %d: %s\n",
					  (const char*)dst, nresult, strTmp);
		}

		memFree(pBuffer);
	}

	return nresult;
}

/*
 * Get a content length based on received headers
 *
 * Return:
 * 		-1 		undefined
 * 		n		content length, bytes (may be 0)
 */
int CHttpContainer::getContentLength() const
{
	const char	*s;
	CString 	strContentLength;
	int 		code, nLength = -1, n;
	result_t	nresult;

	if ( getHeader(strContentLength, HTTP_CONTENT_LENGTH) )  {
		nresult = strContentLength.getNumber(n);
		if ( nresult == ESUCCESS )  {
			return n;
		}
	}

	if ( !m_strStart.isEmpty() )  {
		s = m_strStart;

		SKIP_CHARS(s, " \t");
		if ( _tstrlen(s) > 5 && _tmemcmp(s, "HTTP/", 5) == 0 )  {
			/*
			 * This is response and no context-length header
			 */
			SKIP_NON_CHARS(s, " \t");
			SKIP_CHARS(s, " \t");

			code = _tatoi(s);
			if ( code == 204 || code == 304 || (code/100) == 1 )  {
				/*
				 * Assume no body for response codes
				 * 	204, 304, 1xx
				 */
				nLength = 0;
			}
		}
		else {
			/*
			 * This is request and no context-length header
			 */
		}
	}

	return nLength;
}

result_t CHttpContainer::receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr)
{
	char		strBuf[2048];
	CString		strTransferEncoding;
	hr_time_t	hrNow;
	size_t		length;
	int 		contentLength = -1;
	boolean_t	bDone = FALSE;
	result_t	nresult = ESUCCESS;

	enum {
		RECV_START, RECV_HEADER, RECV_BODY
	} stage;

	shell_assert(!pSrcAddr);

	stage = RECV_START;
	hrNow = hr_time_now();

	while ( nresult == ESUCCESS && !bDone )  {
		switch( stage )  {
			case RECV_START:
				length = sizeof(strBuf);
				nresult = socket.receiveLine(strBuf, &length, HTTP_EOL,
										hr_timeout(hrNow, hrTimeout));
				if ( nresult == ESUCCESS )  {
					log_trace(L_HTTP_CONT, "[http_cont] http code: '%s'", strBuf);

					m_strStart = strBuf;
					m_strStart.rtrim(HTTP_EOL);
					stage = RECV_HEADER;
				}
				else {
					if ( nresult != ECANCELED && !IS_NETWORK_CONNECTION_CLOSED(nresult) ) {
						/* ECANCELED - forced by used to stop receiving */
						log_error(L_GEN, "[http_cont] failed to receive start line, result %d\n",
								  	nresult);
					}
				}
				break;

			case RECV_HEADER:
				length = sizeof(strBuf);
				nresult = socket.receiveLine(strBuf, &length, HTTP_EOL,
											 hr_timeout(hrNow, hrTimeout));
				if ( nresult == ESUCCESS )  {
					log_trace(L_HTTP_CONT, "[http_cont] http header: '%s'", strBuf);

					if ( _tstrcmp(strBuf, HTTP_EOL) == 0 )  {
						/*
						 * Received a header/body separator
						 */
						contentLength = getContentLength();
						//if ( contentLength == 0 )  {
						//	bDone = TRUE;
						//}
						stage = RECV_BODY;
					}
					else {
						/*
						 * Receive a header
						 */
						appendHeader(strBuf);
					}
				}
				else {
					log_error(L_GEN, "[http_cont] failed to receive a header, result %d\n", nresult);
				}
				break;

			case RECV_BODY:
				if ( contentLength >= 0 )  {
					/* Content-Length was specified */
					if ( contentLength > (int)m_nCurSize )  {
						length = sh_min(sizeof(strBuf), contentLength-m_nCurSize);
						nresult = socket.receive(strBuf, &length, CSocket::readFull,
												 hr_timeout(hrNow, hrTimeout));
						if ( nresult == ESUCCESS )  {
							appendBody(strBuf, length);
							if ( contentLength == (int)m_nCurSize )  {
								bDone = TRUE;
							}
						}
						else {
							log_error(L_GEN, "[http_cont] failed to receive a body, result %d\n", nresult);
						}
					}
					else {
						bDone = TRUE;
					}
				}
				else {
					/* Content-Length was not specified */
					if ( getHeader(strTransferEncoding, HTTP_TRANSFER_ENCODING) &&
							_tstrstr(strTransferEncoding, "chunked") != nullptr ) {
						nresult = receiveChunked(socket, hr_timeout(hrNow, hrTimeout));
						bDone = TRUE;
					}
					else {
						/* It seems no body */
						nresult = ESUCCESS;
						bDone = TRUE;

						/*length = sizeof(strBuf);
						nresult = socket.receive(strBuf, &length, 0, hr_timeout(hrNow, hrTimeout));
						if ( nresult == ESUCCESS )  {
							appendBody(strBuf, length);
						}
						else {
							if ( length != 0 )  {
								appendBody(strBuf, length);
							}
							nresult = ESUCCESS;
							bDone = TRUE;
						}*/
					}
				}
				break;
		}
	}

	if ( nresult == ESUCCESS )  {
		log_trace(L_HTTP_CONT, "[http_cont] http body length: %d bytes\n", m_nCurSize);
	}

	return nresult;
}

result_t CHttpContainer::receiveChunked(CSocket& socket, hr_time_t hrTimeout)
{
	char 		strBuffer[4096];
	char*		strBufferExt = nullptr, *tbuf;
	size_t		nBufferExtSize = 0, length;
	int			nChunkLength;
	hr_time_t	hrStart = hr_time_now();
	result_t 	nresult;

	/* Receive chunks */
	while ( true )  {
		length = sizeof(strBuffer);
		nresult = socket.receiveLine(strBuffer, &length, HTTP_EOL, hr_timeout(hrStart, hrTimeout));
		if ( nresult != ESUCCESS )  {
			break;
		}

		log_trace(L_HTTP_CONT, "[http_cont] chunk length line: '%s'\n", strBuffer);

		CString		strSize(strBuffer);

		strSize.ltrim(" \t");
		strSize.rtrim(" \t" HTTP_EOL "\0");
		nresult = strSize.getNumber(nChunkLength, 16);
		if ( nresult != ESUCCESS ) {
			log_error(L_GEN, "[http_cont] invalid chunk hexadecimal number '%s'\n", strSize.cs());
			break;
		}

		if ( nChunkLength < 0 || nChunkLength > 1024*1024*128 )  {
			log_error(L_GEN, "[http_cont] invalid chunk length %ld'\n", nChunkLength);
			break;
		}

		if ( nChunkLength == 0 )  {
			break;	/* last chunk */
		}

		if ( (nChunkLength+HTTP_EOL_LEN) < (int)(sizeof(strBuffer)-1) )  {
			length = nChunkLength+HTTP_EOL_LEN;
			tbuf = strBuffer;
			nresult = socket.receive(strBuffer, &length, CSocket::readFull,
									 	hr_timeout(hrStart, hrTimeout));
		}
		else {
			if ( (int)nBufferExtSize >= (nChunkLength+HTTP_EOL_LEN+1) )  {
				length = nChunkLength+HTTP_EOL_LEN;
				tbuf = strBufferExt;
				nresult = socket.receive(strBufferExt, &length, CSocket::readFull,
										 	hr_timeout(hrStart, hrTimeout));
			}
			else {
				strBufferExt = (char*)memRealloc(strBuffer, nChunkLength+HTTP_EOL_LEN+1);
				if ( strBufferExt != nullptr )  {
					nBufferExtSize = nChunkLength+HTTP_EOL_LEN+1;
					length = nChunkLength+HTTP_EOL_LEN;
					tbuf = strBufferExt;
					nresult = socket.receive(strBufferExt, &length, CSocket::readFull,
											 	hr_timeout(hrStart, hrTimeout));
				}
				else {
					log_error(L_GEN, "[http_cont] memory allocation failure, %lu bytes\n",
							  			nChunkLength+HTTP_EOL_LEN+1);
					nresult = ENOMEM;
				}
			}
		}

		if ( nresult != ESUCCESS )  {
			break;
		}

		log_trace(L_HTTP_CONT, "[http_cont] received chunk: '%s'\n", tbuf);
		appendBody(tbuf, length-HTTP_EOL_LEN);
	}

	if ( nresult == ESUCCESS )  {
		/* Skip trailers */
		while ( true )  {
			length = sizeof(strBuffer);
			nresult = socket.receiveLine(strBuffer, &length, HTTP_EOL, hr_timeout(hrStart, hrTimeout));
			if ( nresult != ESUCCESS )  {
				break;
			}

			CString		strLine(strBuffer);

			strLine.ltrim(" \t");
			strLine.rtrim(" \t" HTTP_EOL "\0");
			if ( strLine.isEmpty() )  {
				break;
			}

			log_trace(L_HTTP_CONT, "[http_cont] received trailer (dropped): '%s'\n", strBuffer);
		}
	}

	if ( strBufferExt != nullptr )  {
		memFree(strBufferExt);
		strBufferExt = nullptr;
	}

	return nresult;
}


/*******************************************************************************
 * Debugging support
 */

void CHttpContainer::dump(const char* strPref) const
{
	const char*		strContent;

	if ( m_nCurSize < 1024 )  {
		strContent = m_pBody ? (const char*)m_pBody : "";
	}
	else {
		strContent = "<<Content skpped>>";
	}

	/*log_dump("%sHttp-Container (body length %u bytes): %s\n",
			 strPref, m_nCurSize, m_strStart.cs());
	log_dump(" => headers: %s\n", m_strHeader.cs());
	log_dump(" => content: %s\n", strContent);*/
	log_dump("%sHttp-Container (body length %u bytes):\n%s\n%s\n%s\n",
			 strPref, m_nCurSize, m_strStart.cs(), m_strHeader.cs(), strContent);
}

void CHttpContainer::getDump(char* strBuf, size_t length) const
{
	_tsnprintf(strBuf, length, "http[%u] %s\n", (unsigned int)m_nCurSize, m_strStart.cs());
}
