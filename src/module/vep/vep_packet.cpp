/*
 *  Carbon/Vep module
 *  Verinet Exchange Protocol packet
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 04.05.2015 13:34:56
 *      Initial revision.
 */

#include "shell/error.h"

#include "carbon/logger.h"
#include "carbon/memory.h"
#include "vep/vep.h"


/*******************************************************************************
 * CVepPacket class
 */

CVepPacket::CVepPacket() :
	m_pPack(m_inBuffer),
	m_nSize(sizeof(m_inBuffer))
{
	_tbzero(m_pPack, VEP_PACKET_HEAD_SZ);
}

CVepPacket::~CVepPacket()
{
	clear();
}

CVepPacket::CVepPacket(vep_packet_type_t type)
{
	create(type);
}

/*
 * Copy constructor
 */
CVepPacket::CVepPacket(const CVepPacket& packet) :
	m_pPack(m_inBuffer),
	m_nSize(sizeof(m_inBuffer))
{
	if ( !packet.isInline() )  {
		uint8_t*	pBuffer;

		pBuffer = (uint8_t*)memAlloc(packet.m_nSize);
		if ( pBuffer )  {
			m_pPack = pBuffer;
			m_nSize = packet.m_nSize;
		}
		else {
			throw std::bad_alloc();
		}
	}

	UNALIGNED_MEMCPY(m_pPack, packet.m_pPack, packet.getSize());
}

/*
 * Create an empty (data length = 0) packet of the specified type
 *
 * 		type		packet type
 */
void CVepPacket::create(vep_packet_type_t type)
{
	vep_packet_head_t*	pHeader;

	clear();

	pHeader = (vep_packet_head_t*)m_pPack;
	pHeader->type = type;
	/*pHeader->length = 0;*/
}

/*
 * Clear packet (all data including header is zeroed)
 */
void CVepPacket::clear()
{
	if ( !isInline() )  {
		memFree(m_pPack);
	}

	m_pPack = m_inBuffer;
	m_nSize = sizeof(m_inBuffer);
	_tbzero(m_pPack, VEP_PACKET_HEAD_SZ);
}

/*
 * Expand packet buffer for the new data
 *
 * 		nDelta			new data size, bytes
 *
 * Return: ESUCCESS, ENOMEM, E2BIG
 */
result_t CVepPacket::expandBuffer(size_t nSize)
{
	vep_packet_head_t*	pHead = (vep_packet_head_t*)m_pPack;
	size_t		newSize, size;
	uint8_t*	pBuf;

	newSize = nSize + VEP_PACKET_HEAD_SZ+pHead->length;
	size = m_nSize;

	shell_assert(size < newSize);
	while ( size < newSize )  {
		size <<= 2;
		if ( size >= (1024*1024) )  {
			break;
		}
	}

	if ( size < newSize )  {
		size = PAGE_ALIGN(newSize);
	}

	if ( isInline() )  {
		log_debug(L_NET_FL, "[vep_pack] alloc: inline buf => ool buf %d bytes\n", size);
	}
	else {
		log_debug(L_NET_FL, "[vep_packet] realloc: %d => %d bytes\n", m_nSize, size);
	}

	pBuf = (uint8_t*)memRealloc(isInline() ? NULL : m_pPack, size);
	if ( !pBuf )  {
		log_error(L_GEN, "[vep_packet] out of memory allocating %u bytes\n", size);
		return ENOMEM;
	}

	if ( isInline() )  {
		UNALIGNED_MEMCPY(pBuf, m_inBuffer, VEP_PACKET_HEAD_SZ+pHead->length);
	}

	m_pPack = pBuf;
	m_nSize = size;

	return ESUCCESS;
}

/*
 * Insert data to the packet
 *
 * 		pData		data pointer
 * 		nSize		data size, bytes
 *
 * Return:
 * 		ESUCCESS	success
 * 		ENOMEM		memory shortage
 * 		ENOSPC		data overflow
 */
result_t CVepPacket::putData(const void* pData, size_t nSize)
{
	vep_packet_head_t*	pHead = (vep_packet_head_t*)m_pPack;
	size_t				freeLen;
	result_t			nresult = ESUCCESS;

	shell_assert(pData);
	shell_assert(pHead->type != VEP_PACKET_TYPE_NULL);

	if ( nSize < 1 )  {
		return ESUCCESS;
	}

	if ( pHead->type == VEP_PACKET_TYPE_NULL )  {
		return EINVAL;
	}

	if ( (nSize+VEP_PACKET_HEAD_SZ+pHead->length) > VEP_PACKET_MAX_SIZE )  {
		log_error(L_GEN, "[vep_pack] packet data overflow\n");
		return ENOSPC;
	}

	/*
	 * Put body
	 */
	freeLen = m_nSize - (VEP_PACKET_HEAD_SZ+pHead->length);
	shell_assert(freeLen < VEP_PACKET_MAX_SIZE);

	if ( freeLen < nSize ) {
		nresult = expandBuffer(nSize);
	}

	if ( nresult == ESUCCESS )  {
		UNALIGNED_MEMCPY(m_pPack + VEP_PACKET_HEAD_SZ + pHead->length, pData, nSize);
		pHead = (vep_packet_head_t*)m_pPack;
		pHead->length += nSize;
	}

	return nresult;
}

/*
 * Make a completed packet (setup correct length)
 *
 * Return:
 * 		ESUCCESS	success
 * 		EFAULT		packet data too small
 */
result_t CVepPacket::finalise()
{
	return isValid() ? ESUCCESS : EFAULT;
}

/*
 * Check if a packet is valid
 *
 * Return: TRUE/FALSE
 */
boolean_t CVepPacket::isValid() const
{
	vep_packet_head_t*	pHead = (vep_packet_head_t*)m_pPack;

	shell_assert((pHead->length+VEP_PACKET_HEAD_SZ) <= m_nSize);

	if ( pHead->type == VEP_PACKET_TYPE_NULL )  {
		log_debug(L_GEN, "[vep_pack] no packet type was specified\n");
		return FALSE;
	}

	if ( (pHead->length+VEP_PACKET_HEAD_SZ) >= VEP_PACKET_MAX_SIZE )  {
		log_debug(L_GEN, "[vep_pack] packet too big, %u bytes\n",
				  pHead->length+VEP_PACKET_HEAD_SZ);
		return FALSE;
	}

	return TRUE;
}


/*******************************************************************************
 * Debugging support
 */

void CVepPacket::dumpData(const char* strPref) const
{
	int 	dataLen = (int)(m_nSize-sizeof(vep_packet_head_t));
	int 	length = MIN(8, dataLen);

	if ( length > 0 )  {
		log_dump_bin(m_pPack+sizeof(vep_packet_head_t), length,
					 "| %sVep-Packet Data:", strPref);
	}
}

void CVepPacket::dump(vep_container_type_t contType, const char* strPref) const
{
	vep_packet_head_t*	pHead = (vep_packet_head_t*)m_pPack;
	char				strTmp[64];
	size_t				l = sizeof(vep_packet_head_t) + pHead->length;

	log_dump("| %sVep-Packet: inline: %s, full_size: %u, data_size: %u bytes\n",
			 strPref, isInline() ? "YES" : "NO", l, pHead->length);

	/* Packet header */
	CVepContainer::getPacketName(contType, pHead->type, strTmp, sizeof(strTmp));

	log_dump("| %sVep-Packet Header: type: %s (%u), length: %u byte(s)\n",
			 strPref, strTmp, pHead->type, pHead->length);

	dumpData();
}
