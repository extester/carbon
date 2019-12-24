/*
 *  Carbon/Vep module
 *  Verinet Exchange Protocol container
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 27.09.2016 16:36:31
 *      Initial revision.
 */

#include "shell/dec_ptr.h"

#include "carbon/utils.h"
#include "carbon/memory.h"
#include "vep/vep_container_atomic.h"


/*******************************************************************************
 * CVepContainerAtomic class
 */

CVepContainerAtomic::CVepContainerAtomic(size_t nMaxSize) :
	CVepContainer(),
	m_nMaxSize(nMaxSize)
{
	shell_assert(nMaxSize <= VEP_CONTAINER_MAX_SIZE);
}

CVepContainerAtomic::CVepContainerAtomic(vep_container_type_t contType,
										 vep_packet_type_t packType,
										 size_t nMaxSize) :
	CVepContainer(contType, packType),
	m_nMaxSize(nMaxSize)
{
	shell_assert(nMaxSize <= VEP_CONTAINER_MAX_SIZE);
}

CVepContainerAtomic::~CVepContainerAtomic()
{
}
/*
 * Create a copy of the container
 *
 * Return: new container with reference counter 1
 * Raise exception on memory error
 */
CNetContainer* CVepContainerAtomic::clone()
{
	dec_ptr<CVepContainerAtomic>	pContainer = new CVepContainerAtomic(m_nMaxSize);
	size_t							i, count = getPackets();

	pContainer->m_header = m_header;

	for(i=0; i<count; i++)  {
		pContainer->m_arPacket.push_back(new CVepPacket(*m_arPacket[i]));
	}

	pContainer->reference();
	return pContainer;
}

/*
 * Receive a container with timeout
 *
 * 		socket			connected socket
 * 		hrTimeout		maximum receive time
 *
 * Return: ESUCCESS, ETIMEDOUT, ENOMEM, EINVAL, EINTR
 */
result_t CVepContainerAtomic::receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr)
{
	vep_container_head_t*	pHead;
	size_t					size;
	result_t				nresult;

	shell_assert(socket.isOpen());
	shell_assert(isEmpty());

	clear();

	if ( m_nMaxSize <= sizeof(m_inBuffer) ) {
		pHead = (vep_container_head_t*)&m_inBuffer;
	}
	else {
		pHead = (vep_container_head_t*)memAlloc(m_nMaxSize);
		if ( !pHead )  {
			log_error(L_GEN, "[vepa_cont] out of memory\n");
			return ENOMEM;
		}
	}

	size = m_nMaxSize;
	nresult = socket.receive(pHead, &size, 0, hrTimeout, pSrcAddr);
	if ( nresult != ESUCCESS )  {
		freeSerialised(pHead);
		return nresult;
	}

	if ( checkHeader(pHead) ) {
		uint16_t	incrc = pHead->crc, crc;

		pHead->crc = 0;
		crc = crc16(pHead, getHeadSize(pHead)+pHead->length);
		pHead->crc = incrc;
		if ( incrc == crc )  {
			nresult = unserialise(pHead);
		}
		else {
			log_error(L_GEN, "[vepa_cont] container CRC wrong, dropped\n");
			nresult = EFAULT;
		}
	}
	else {
		log_debug(L_GEN, "[vepa_cont] invalid container header received\n");
		freeSerialised(pHead);
		nresult = EINVAL;
	}

	freeSerialised(pHead);

	return nresult;
}
