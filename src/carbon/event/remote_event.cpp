/*
 *  Carbon framework
 *  Remote Events
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 01.08.2016 17:53:40
 *      Initial revision.
 */

#include "carbon/memory.h"
#include "carbon/logger.h"
#include "carbon/event/remote_event.h"

/*******************************************************************************
 * Class CRemoteEvent
 */

void CRemoteEvent::setData(const void* pData, size_t size)
{
	SAFE_FREE(m_pData);
	m_size = 0;

	if ( size > 0 ) {
		m_pData = memAlloc(size);
		if ( !m_pData ) {
			throw std::bad_alloc();
		}
		UNALIGNED_MEMCPY(m_pData, pData, size);
		m_size = size;
	}
}

result_t CRemoteEvent::send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr)
{
	remote_event_head_t		head;
	hr_time_t				hrNow;
	result_t				nresult;

	_tbzero_object(head);
	head.ident = REMOTE_EVENT_IDENT;
	head.length = sizeof(head)+m_size;
	head.type = getType();
	head.pparam = (uint64_t)getpParam();
	head.nparam = (uint64_t)getnParam();
	head.sessId = getSessId();
	head.receiver = (uint64_t)getReceiver();
	head.reply_receiver = (uint64_t)getReplyReceiver();
	copyString(head.reply_rid, getReplyRid(), sizeof(head.reply_rid));

	hrNow = hr_time_now();
	nresult = socket.send(&head, sizeof(head), hrTimeout, dstAddr);
	if ( nresult == ESUCCESS && m_size > 0 )  {
		nresult = socket.send(getData(), m_size, hr_timeout(hrNow, hrTimeout));
	}

	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[remote_event] failed to send event %d, result: %d\n",
				  getType(), nresult);
	}

	return nresult;
}

result_t CRemoteEvent::receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr)
{
	remote_event_head_t	head;
	hr_time_t			hrNow;
	void*				pBuffer = NULL;
	size_t				size;
	result_t			nresult;

	/* Clear event */
	SAFE_FREE(m_pData);
	m_size = 0;

	hrNow = hr_time_now();
	size = sizeof(head);
	nresult = socket.receive(&head, &size, CSocket::readFull, hrTimeout, pSrcAddr);
	if ( nresult == ESUCCESS && head.length > sizeof(head) ) {
		size = head.length-sizeof(head);

		pBuffer = memAlloc(size);
		if ( pBuffer != NULL )  {
			nresult = socket.receive(pBuffer, &size, CSocket::readFull, hr_timeout(hrNow, hrTimeout), pSrcAddr);
			if ( nresult != ESUCCESS )  {
				log_debug(L_GEN, "[remote_event] failed to receive event %d data, result: %d\n",
						  	head.type, nresult);
			}
		}
		else {
			log_error(L_GEN, "[remote_client] failed to allocate %d bytes of memory for event %d body\n",
					  		size, head.type);
			nresult = ENOMEM;
		}
	}
	else {
		if ( nresult != ESUCCESS )  {
			log_debug(L_GEN, "[remote_event] failed to receive event head, result: %d\n",
					  nresult);
		}
	}

	if ( nresult == ESUCCESS ) {
		/* Rebuild event data */
		m_type = head.type;
		m_pParam = (PPARAM)head.pparam;
		m_nParam = (NPARAM)head.nparam;
		m_sessId = head.sessId;
		m_pReceiver = (CEventReceiver*)head.receiver;
		m_pReplyReceiver = (CEventReceiver*)head.reply_receiver;
		m_strReplyRid = head.reply_rid;

		m_pData = pBuffer;
		pBuffer = NULL;
		m_size = size;
	}

	SAFE_FREE(pBuffer);
	return nresult;
}

/*******************************************************************************
 * Debugging support
 */

#if CARBON_DEBUG_DUMP

const char* CRemoteEvent::dumpEvent(char* strBuffer, size_t length) const
{
	return CEvent::dumpEvent(strBuffer, length);
}

void CRemoteEvent::dump(const char* strPref) const
{
	CEvent::dump(strPref);
	log_dump("\t\t session ID: %u, payload data: %d bytes, reply rid: '%s'\n",
			 m_sessId, m_size, m_strReplyRid.cs());
}

#endif /* CARBON_DEBUG_DUMP */

/*******************************************************************************
 * Class CRemoteEventContainer
 */

CNetContainer* CRemoteEventContainer::clone()
{
	dec_ptr<CRemoteEvent>	pEvent = new CRemoteEvent;
	dec_ptr<CRemoteEventContainer>	pContainer = new CRemoteEventContainer(pEvent);

	pEvent->copy(*m_pEvent);
	pContainer->reference();

	return pContainer;
}

#if CARBON_DEBUG_DUMP

void CRemoteEventContainer::dump(const char* strPref) const
{
	shell_unused(strPref);
}

void CRemoteEventContainer::getDump(char* strBuf, size_t length) const
{
	if ( length > 0 ) {
		*strBuf = '\0';
	}
}

#endif /* CARBON_DEBUG_DUMP */
