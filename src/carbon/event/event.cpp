/*
 *  Carbon framework
 *  Event base class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.06.2015 11:29:49
 *      Initial revision.
 */

#include "shell/error.h"

#include "carbon/logger.h"
#include "carbon/event.h"
#include "carbon/event/eventloop.h"


/*******************************************************************************
 * Event receiver class CEventReceiver
 */

CEventReceiver::CEventReceiver(CEventLoop* pOwnerLoop, const char* strName) :
	CObject(strName),
	CListItem(),
	m_pEventLoop(pOwnerLoop)
{
	shell_assert(pOwnerLoop);
	if ( m_pEventLoop ) {
		m_pEventLoop->registerReceiver(this);
	}
}

CEventReceiver::CEventReceiver(const char* strName) :
	CObject(strName),
	CListItem(),
	m_pEventLoop(appMainLoop())
{
	CEventLoop*		pLoop = appMainLoop();

	shell_assert(pLoop);
	if ( pLoop ) {
		pLoop->registerReceiver(this);
	}
}

CEventReceiver::~CEventReceiver()
{
	if ( m_pEventLoop )  {
		m_pEventLoop->unregisterReceiver(this);
	}
}

/*******************************************************************************
 * Event class CEvent
 */

#if CARBON_DEBUG_EVENT

/*
 * Fill buffer with an event name
 *
 * 		strBuffer		output buffer
 * 		length			output buffer length
 */
void CEvent::getEventName(char* strBuffer, size_t length) const
{
	CEventStringTable::getEventName(this, strBuffer, length);
}

#endif /* CARBON_DEBUG_EVENT */

#if CARBON_DEBUG_DUMP

/*
 * Dump an event parameters
 *
 * 		strBuffer		output buffer
 * 		length			output buffer max length
 *
 * Return: string
 */
const char* CEvent::dumpEvent(char* strBuffer, size_t length) const
{
	char	tmp[128];

	getEventName(tmp, sizeof(tmp));
	_tsnprintf(strBuffer, length, "%lX: %-24s pparam=%lXh, nparam=%lu, %s",
			 (long unsigned int)this, tmp, (unsigned long int)m_pParam,
			 (unsigned long int)m_nParam, m_strDesc);
	return strBuffer;
}

void CEvent::dump(const char* strPref) const
{
	char 	stmp[256];

	dumpEvent(stmp, sizeof(stmp));
	log_dump("%s%s\n", strPref, stmp);
}

#endif /* CARBON_DEBUG_DUMP */


/*******************************************************************************
 * CEventSerialise class
 */
#if 0
result_t CEventSerialise::serialiseData(const serialise_data_t* arData, int count,
										void** ppBuffer, size_t* pSize) const
{
	size_t		size = 0;
	int			i;
	uint8_t		*pBuffer, *p;

	for(i=0; i<count; i++)  {
		size += arData[i].size;
	}

	pBuffer = (uint8_t*)memAlloc(size);
	if ( pBuffer == 0 )  {
		log_error(L_GEN, "[event_ser] memory allocation %d bytes failed\n", size);
		*ppBuffer = NULL;
		*pSize = 0;
		return ENOMEM;
	}

	p = pBuffer;
	for(i=0; i<count; i++)  {
		UNALIGNED_MEMCPY(p, arData[i].pData, arData[i].size);
		p += arData[i].size;
	}

	*pSize = size;
	*ppBuffer = pBuffer;

	return ESUCCESS;
}

result_t CEventSerialise::unserialiseData(const serialise_data_t* arData, int count,
										  const void* pBuffer, size_t size)
{
	size_t		rsize = 0;
	int 		i;
	uint8_t*	p;

	for(i=0; i<count; i++)  {
		rsize += arData[i].size;
	}

	if ( size != rsize )  {
		log_error(L_GEN, "[event_ser] invalid data size %d, expected %d\n", rsize, size);
		return EINVAL;
	}

	p = (uint8_t*)pBuffer;

	for(i=0; i<count; i++)  {
		UNALIGNED_MEMCPY(arData[i].pData, p, arData[i].size);
		p += arData[i].size;
	}

	return ESUCCESS;
}
#endif
