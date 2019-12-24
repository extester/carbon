/*
 *  Carbon framework
 *  Simple raw container
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 09.09.2016 13:10:53
 *      Initial revision.
 */

#include "shell/shell.h"
#include "shell/debug.h"
#include "shell/dec_ptr.h"

#include "carbon/logger.h"
#include "carbon/memory.h"
#include "carbon/raw_container.h"

#define RAW_CONTAINER_BUFFER_STEP		64

/*******************************************************************************
 * CRawContainer class
 */

/*
 * Constructor
 *
 * 		nMaxSize		maximum data size container can contains, bytes
 */
CRawContainer::CRawContainer(size_t nMaxSize) :
	CNetContainer(),
	m_pBuffer(NULL),
	m_nCurSize(0),
	m_nSize(0),
	m_nMaxSize(nMaxSize)
{
}

/*
 * Destructor
 */
CRawContainer::~CRawContainer()
{
	clear();
}

/*
 * Free container data and allocated resources
 */
void CRawContainer::clear()
{
	SAFE_FREE(m_pBuffer);
	m_nSize = m_nCurSize = 0;
	CNetContainer::clear();
}

/*
 * Make container size big enough to contains nSize bytes
 *
 * 		nSize		minimum container buffer size
 *
 * Return:
 * 		ESUCCESS	success
 * 		ENOMEM		out of memory
 * 		ENOSPC		buffer sized limit reached
 */
result_t CRawContainer::makeBuffer(size_t nSize)
{
	result_t	nresult = ENOSPC;

	if ( nSize <= m_nMaxSize )  {
		size_t 		n, newSize;
		uint8_t*	pBuffer;

		n = RAW_CONTAINER_BUFFER_STEP*4;
		if ( nSize <= n ) {
			newSize = ALIGN(nSize, RAW_CONTAINER_BUFFER_STEP);
		} else
		if ( nSize <= (n*4) )  {
			newSize = ALIGN(nSize, n);
		} else {
			newSize = ALIGN(nSize, 1024);
		}

		newSize = MIN(newSize, m_nMaxSize);
		if ( newSize > m_nSize ) {
			pBuffer = (uint8_t*)memRealloc(m_pBuffer, newSize);
			if ( pBuffer ) {
				m_pBuffer = pBuffer;
				m_nSize   = newSize;
				nresult   = ESUCCESS;
			} else {
				log_error(L_GEN, "[raw_cont] out of memory, %d bytes\n", newSize);
				nresult = ENOMEM;
			}
		}
		else {
			nresult = ESUCCESS;
		}
	}

	return nresult;
}

/*
 * Copy container
 *
 * Return: new container object address or 0 on any error
 */
CNetContainer* CRawContainer::clone()
{
	dec_ptr<CRawContainer>	pContainer = new CRawContainer(m_nMaxSize);
	result_t				nresult = ESUCCESS;

	if ( m_nCurSize > 0 ) {
		nresult = pContainer->putData(m_pBuffer, m_nCurSize);
	}

	if ( nresult == ESUCCESS )  {
		pContainer->reference();
		return (CNetContainer*)(CRawContainer*)pContainer;
	}

	return 0;
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
 * 		ENOSPC		buffer sized limit reached
 */
result_t CRawContainer::putData(const void* pData, size_t nSize)
{
	size_t		freeLen, rSize;
	result_t	nresult;

	if ( nSize < 1 )  {
		return ESUCCESS;
	}

	freeLen = m_nMaxSize - m_nCurSize;
	if ( freeLen == 0 )  {
		log_debug(L_GEN, "[raw_cont] buffer overflow, %u bytes of data dropped\n", nSize);
		return ENOSPC;
	}

	rSize = nSize;
	if ( freeLen < nSize )  {
		log_debug(L_GEN, "[raw_cont] buffer overflow, %u of %u bytes are dropped\n",
							nSize, nSize-freeLen);
		rSize = freeLen;
	}

	nresult = makeBuffer(m_nCurSize+rSize);
	if ( nresult == ESUCCESS )  {
		UNALIGNED_MEMCPY(&m_pBuffer[m_nCurSize], pData, rSize);
		m_nCurSize += rSize;
	}

	return nresult;
}

/*
 * Send a container
 *
 * 		socket		open socket object
 * 		hrTimeout	maximum send time
 * 		dstAddr		destination address (optional, for UDP)
 *
 * 	Return:
 * 		ESUCCESS	all data have been sent
 * 		EAGAIN		can't send all data at the moment, repeat again
 * 		EINTR		sending was interrupted by signal
 * 		...
 */
result_t CRawContainer::send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr)
{
	result_t	nresult = ESUCCESS;

	if ( m_nCurSize > 0 )  {
		size_t	size;

		size = m_nCurSize;
		nresult = socket.send(m_pBuffer, &size, hrTimeout, dstAddr);
		if ( nresult != ESUCCESS )  {
			log_debug(L_GEN, "[raw_cont] failed to send container, result %d\n", nresult);
		}
	}
	else {
		log_warning(L_GEN, "[raw_cont] container has no data\n");
	}

	return nresult;
}

/*
 * Receive a container
 *
 * 		socket		open socket object
 * 		hrTimeout	maximum receive time
 * 		pSrcAddr	container source network address (optional, may be NULL)
 *
 * 	Return:
 * 		ESUCCESS	all data have been sent
 * 		EAGAIN		can't send all data at the moment, repeat again
 * 		EINTR		sending was interrupted by signal
 * 		...
 */
result_t CRawContainer::receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr)
{
	result_t	nresult;

	nresult = makeBuffer(m_nMaxSize);
	if ( nresult == ESUCCESS )  {
		size_t		size;

		m_nCurSize = 0;
		size = m_nMaxSize;
		nresult = socket.receive(m_pBuffer, &size, 0, hrTimeout, pSrcAddr);
		if ( nresult == ESUCCESS ) {
			m_nCurSize = size;
		}
		else {
			if ( nresult != ETIMEDOUT && nresult != ECANCELED )  {
				log_debug(L_GEN, "[raw_cont] failed to receive a container, result %d\n",
						  nresult);
			}
		}
	}

	return nresult;
}

/*******************************************************************************
 * Debugging support
 */

void CRawContainer::dump(const char* strPref) const
{
	log_dump_bin(m_pBuffer, m_nCurSize, "| %sRaw-Container: nCurSize: %u, nMaxSize: %u\n",
			 strPref, m_nCurSize, m_nMaxSize);
}

void CRawContainer::getDump(char* strBuf, size_t length) const
{
	int		len;

	len = _tsnprintf(strBuf, length, "raw[%u] ", (unsigned)m_nCurSize);
	if ( (size_t)len < length )  {
		getDumpHex(&strBuf[len], length-len, m_pBuffer, MIN(m_nCurSize, 16));
	}
}
