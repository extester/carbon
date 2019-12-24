/*
 *  Carbon framework
 *  Simple text (string-oriented) container
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 19.04.2015 16:35:06
 *      Initial revision.
 */

#include "shell/shell.h"

#include "carbon/logger.h"
#include "carbon/memory.h"
#include "carbon/text_container.h"


/*******************************************************************************
 * CTextContainer class
 */

CTextContainer::CTextContainer(size_t nMaxSize, const char* strEol) :
	CNetContainer(),
	m_pBuffer(m_inBuffer),
	m_nSize(0),
	m_nCurSize(0),
	m_nMaxSize(nMaxSize),
	m_strEol(strEol)
{
	m_inBuffer[0] = '\0';
}

CTextContainer::~CTextContainer()
{
	clear();
}

void CTextContainer::clear()
{
	if ( !isInline() )  {
		memFree(m_pBuffer);
		m_pBuffer = m_inBuffer;
		m_nCurSize = 0;
	}
	m_nSize = 0;
	m_inBuffer[0] = '\0';

	CNetContainer::clear();
}

result_t CTextContainer::expandBuffer(size_t nDelta)
{
	size_t	freeSize, nNewSize, nAllocSize;
	char*	pBuf;

	shell_assert((m_nSize+nDelta) <= m_nMaxSize);

	if ( isInline() )  {
		freeSize = sizeof(m_inBuffer) - m_nSize;
		if ( freeSize >= (nDelta+1) )  {
			return ESUCCESS;
		}
	}
	else {
		shell_assert(m_nCurSize >= m_nSize);
		freeSize = m_nCurSize - m_nSize;
		if ( freeSize >= (nDelta+1) )  {
			return ESUCCESS;
		}
	}

	/* Calculate new allocation size */
	nNewSize = m_nSize + nDelta+1;

	nAllocSize = m_nCurSize ? (m_nCurSize*2) : (TEXT_CONTAINER_INSIZE*2);
	nAllocSize = MAX(nAllocSize, nNewSize);
	nAllocSize = PAGE_ALIGN(nAllocSize);

	if ( isInline() )  {
		log_debug(L_NET_FL, "[text_cont] alloc: inline buf => ool buf %d bytes\n", nAllocSize);
	}
	else {
		log_debug(L_NET_FL, "[text_cont] realloc: %d => %d bytes\n", m_nCurSize, nAllocSize);
	}

	pBuf = (char*)memRealloc(isInline() ? NULL : m_pBuffer, nAllocSize);
	if ( !pBuf )  {
		log_error(L_GEN, "[text_container] out of memory allocating %u bytes\n", nAllocSize);
		return ENOMEM;
	}

	if ( isInline() )  {
		UNALIGNED_MEMCPY(pBuf, m_inBuffer, m_nSize);
	}

	m_pBuffer = pBuf;
	m_pBuffer[m_nSize] = '\0';
	m_nCurSize = nAllocSize;

	return ESUCCESS;
}

/*
 * Write data to the container
 *
 * 		pData		data buffer
 * 		nSize		data size, bytes
 *
 * 	Return:
 * 		ESUCCESS	success
 * 		ENOMEM		out of memory
 * 		ENOSPC		limit reached
 */
result_t CTextContainer::putData(const void* pData, size_t nSize)
{
	size_t		freeLen, rSize;
	result_t	nresult;

	if ( nSize < 1 )  {
		return ESUCCESS;
	}

	freeLen = m_nMaxSize - m_nSize;
	if ( freeLen == 0 )  {
		log_debug(L_GEN, "[text_container] buffer is full, data %u bytes is dropped\n", nSize);
		return ENOSPC;
	}

	rSize = nSize;
	if ( freeLen < nSize )  {
		log_debug(L_GEN, "[text_container] buffer overflow, %d of %d bytes are dropped\n",
							nSize, nSize-freeLen);
		rSize = freeLen;
	}

	nresult = expandBuffer(rSize);
	if ( nresult == ESUCCESS )  {
		UNALIGNED_MEMCPY(&m_pBuffer[m_nSize], pData, rSize);
		m_nSize += rSize;
		m_pBuffer[m_nSize] = '\0';
	}

	return nresult;
}

result_t CTextContainer::putData(const char* strData)
{
	result_t	nresult;

	nresult = putData(strData, _tstrlen(strData));
	return nresult;
}

/*
 * Send container in asynchronous mode
 *
 * 		socket		open socket object
 * 		pOffset		send offset [in/out]
 *
 * 	Return:
 * 		ESUCCESS	all data have been sent
 * 		EAGAIN		can't send all data at the moment, repeat again
 * 		EINTR		sending was interrupted by signal
 * 		...
 */
result_t CTextContainer::send(CSocketAsync& socket, uint32_t* pOffset)
{
	size_t		size;
	result_t	nresult;

	if ( m_nSize == 0 || *pOffset == m_nSize )  {
		return ESUCCESS;
	}

	if ( *pOffset > m_nSize )  {
		log_debug(L_GEN, "[text_container] send offset is out of range, "
					"offset %u, data size %u\n", *pOffset, m_nSize);
		return EINVAL;
	}

	size = m_nSize - (*pOffset);
	nresult = socket.send(&m_pBuffer[*pOffset], &size);
	*pOffset += size;

	return nresult;
}

result_t CTextContainer::receive(CSocketAsync& socket)
{
	char		tmpBuf[TEXT_CONTAINER_INSIZE];
	size_t		size, n, lenEol;
	result_t	nresult, nr;

	nresult = ESUCCESS;
	lenEol = _tstrlen(m_strEol);
	shell_assert(sizeof(tmpBuf) > lenEol);

	log_debug(L_NET_FL, "[text_cont] current size %d, async receiving...\n", m_nSize);

	while( nresult == ESUCCESS )  {
		if ( m_nSize >= lenEol && _tmemmem(m_pBuffer, m_nSize, m_strEol, lenEol) != NULL ) {
			log_debug(L_NET_FL, "[text_cont] current size %d, detected EOL, stop\n", m_nSize);
			break;
		}

		n = m_nSize > lenEol ? lenEol : m_nSize;
		UNALIGNED_MEMCPY(tmpBuf, &m_pBuffer[m_nSize-n], n);
		size = sizeof(tmpBuf)-n-1;
		nresult = socket.receiveLine(tmpBuf, n, &size, m_strEol);
		log_debug(L_NET_FL, "[text_cont] received size %d, nresult: %d\n", size, nresult);
		if ( size > 0 )  {
			nr = putData(tmpBuf+n, size);
			/*log_debug(L_NET_FL, "[text_cont] content '%s', put result: %d\n", m_pBuffer, nr);*/
			nresult_join(nresult, nr);
		}
	}

	return nresult;
}

/*******************************************************************************
 * Debugging support
 */

void CTextContainer::dump(const char* strPref) const
{
	log_dump("| %sText-Container: nSize: %u, nCurSize: %u, nMaxSize: %u\n",
			 strPref, m_nSize, m_nCurSize, m_nMaxSize);
}
