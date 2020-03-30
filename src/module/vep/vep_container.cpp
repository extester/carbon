/*
 *  Carbon/Vep module
 *  Verinet Exchange Protocol container
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 04.05.2015 19:32:07
 *      Initial revision.
 */

#include <new>

#include "shell/dec_ptr.h"

#include "carbon/logger.h"
#include "carbon/utils.h"
#include "carbon/memory.h"
#include "vep/vep.h"

#define VEP_PACKETS_MAX					16


/*******************************************************************************
 * CVepContainer class
 */

vep_container_names_t 	CVepContainer::m_tableName;
CMutex					CVepContainer::m_tableNameLock;


CVepContainer::CVepContainer() :
	CNetContainer()
{
	m_arPacket.reserve(VEP_PACKETS_MAX);
	clear();
}

CVepContainer::CVepContainer(vep_container_type_t contType, vep_packet_type_t packType) :
	CNetContainer()
{
	m_arPacket.reserve(VEP_PACKETS_MAX);
	create(contType, packType);
}

CVepContainer::~CVepContainer()
{
	clear();
}

/*
 * Clears object and deletes all packets
 */
void CVepContainer::clear()
{
	size_t		i, count;

	count = getPackets();

	for(i=0; i<count; i++)  {
		SAFE_DELETE(m_arPacket[i]);
	}

	m_arPacket.clear();
	_tbzero_object(m_header);
}

/*
 * Create a empty container/packet
 *
 * 		contType	container type, VEP_CONTAINER_xxx
 * 		packType	embedded packet type (do not create packet if VEP_PACKET_TYPE_NULL)
 *
 * Return: ESUCCESS, ...
 */
result_t CVepContainer::create(vep_container_type_t contType, vep_packet_type_t packType)
{
	result_t	nresult = ESUCCESS;

	/*
	 * Delete previous content
	 */
	clear();

	UNALIGNED_MEMCPY(&m_header.ident, VEP_IDENT, sizeof(m_header.ident));
	m_header.version = VEP_VERSION;
	m_header.type = contType;
	/*m_header.flags = 0;*/
	/*m_header.length = 0;*/
	/*m_header.crc = 0;*/

	if ( packType != VEP_PACKET_TYPE_NULL ) {
		nresult = insertPacket(packType);
	}

	return nresult;
}

/*
 * Create a copy of the container
 *
 * Return: new container with reference counter 1
 * Raise exception on memory error
 */
CNetContainer* CVepContainer::clone()
{
	dec_ptr<CVepContainer>	pContainer = new CVepContainer;
	size_t					i, count = getPackets();

	pContainer->m_header = m_header;

	for(i=0; i<count; i++)  {
		pContainer->m_arPacket.push_back(new CVepPacket(*m_arPacket[i]));
	}

	pContainer->reference();
	return pContainer;
}

size_t CVepContainer::getHeadSize(const vep_container_head_t* pHead) const
{
	const vep_container_head_t*	pH;
	size_t						size;

	pH = pHead ? pHead : &m_header;

	size = VEP_CONTAINER_HEAD_CONST_SIZE +
		   ((pH->flags&VEP_CONT_ADDR)!=0)*sizeof(vep_addr_t) +
		   ((pH->flags>>VEP_CONT_RESERVED_SHIFT)&VEP_CONT_RESERVED_MASK)*sizeof(uint32_t);

	return size;
}

size_t CVepContainer::getDataSize() const
{
	uint32_t	size;
	size_t		i, count;

	size = 0;
	count = getPackets();

	for(i=0; i<count; i++)  {
		size += m_arPacket[i]->getSize();
	}

	return size;
}

/*
 * Append a empty packet
 *
 * 		packType		packet type to append
 *
 * Return: ESUCCESS, ENOSPC, ENOMEM
 */
result_t CVepContainer::insertPacket(vep_packet_type_t packType)
{
	CVepPacket*		pPacket;
	result_t		nresult;

	if ( packType == VEP_PACKET_TYPE_NULL )  {
		log_error(L_GEN, "[vep_cont] no packet type specified\n");
		return EINVAL;
	}

	if ( getPackets() >= VEP_PACKETS_MAX ) {
		log_error(L_GEN, "[vep_cont] too many packets in container\n");
		return ENOSPC;
	}

	pPacket = 0;
	nresult = ESUCCESS;
	try {
		pPacket = new CVepPacket();
		pPacket->create(packType);
		m_arPacket.push_back(pPacket);
	}
	catch (const std::bad_alloc& exc)  {
		nresult = ENOMEM;
		log_error(L_GEN, "[ver_cont] out of memory\n");
	}

	if ( nresult != ESUCCESS )  {
		SAFE_DELETE(pPacket);
	}

	return nresult;
}

/*
 * Insert new packet to the container
 *
 * 		packType		packet type
 * 		pData			packet header pointer (type and length are ignored)
 * 		nSize			packet size excluding header
 *
 * Return: ESUCCESS, ENOSPC, ENOMEM
 */
result_t CVepContainer::insertPacket(vep_packet_type_t packType,
									 const void* pData, size_t nSize)
{
	result_t	nresult;

	shell_assert(packType != VEP_PACKET_TYPE_NULL);
	shell_assert(pData);
	//shell_assert(nSize >= VEP_PACKET_HEAD_SZ);

	if ( packType == VEP_PACKET_TYPE_NULL || (pData == NULL && nSize > 0) )  {
		log_error(L_GEN, "[vep_cont] wrong data\n");
		return EINVAL;
	}

	nresult = insertPacket(packType);
	if ( nresult == ESUCCESS )  {
//		nresult = m_arPacket[getPackets()-1]->putData((uint8_t*)pData+VEP_PACKET_HEAD_SZ,
//													  nSize-VEP_PACKET_HEAD_SZ);
		nresult = m_arPacket[getPackets()-1]->putData(pData, nSize);
		if ( nresult != ESUCCESS )  {
			deletePacket(getPackets()-1);
		}
	}

	return nresult;
}

void CVepContainer::deletePacket(size_t index)
{
	if ( index < getPackets() )  {
		delete m_arPacket[index];
		m_arPacket.erase(m_arPacket.begin()+index);
	}
}


boolean_t CVepContainer::checkHeader(const vep_container_head_t* pHead) const
{
	if ( _tmemcmp(pHead->ident, &VEP_IDENT, sizeof(pHead->ident)) != 0 )  {
		char	tmp[sizeof(pHead->ident)+1];

		UNALIGNED_MEMCPY(tmp, pHead->ident, sizeof(pHead->ident));
		tmp[sizeof(pHead->ident)] = '\0';
		log_debug(L_GEN, "[vep_cont] invalid ident '%s'\n", tmp);
		return FALSE;
	}

	if ( pHead->version != VEP_VERSION )  {
		log_debug(L_GEN, "[vep_cont] unsupported container version %u, expected %u\n",
				  pHead->version, VEP_VERSION);
		return FALSE;
	}

	return TRUE;
}


boolean_t CVepContainer::isValid() const
{
	size_t	size;

	if ( !checkHeader(&m_header) )  {
		return FALSE;
	}

	size = getDataSize();
	if ( size != m_header.length )  {
		log_debug(L_GEN, "[vep_cont] invalid container size %d, correct %d\n",
				  m_header.length, size);
		return FALSE;
	}

	/*
	 * Checking the packets
	 */
	boolean_t	bValid = TRUE;
	size_t		count = getPackets(), i;

	if ( count < 1 || count > VEP_PACKETS_MAX )  {
		log_debug(L_GEN, "[vep_cont] invalid packe count %d in container\n", count);
		return FALSE;
	}

	for(i=0; i<count; i++)  {
		if ( !m_arPacket[i]->isValid() )  {
			log_debug(L_GEN, "[vep_cont] packet #%d is invalid\n", i);
			bValid = FALSE;
			break;
		}
	}

	return bValid;
}

result_t CVepContainer::insertData(const void* pData, size_t nSize, size_t index)
{
	result_t	nresult;

	if ( index >= getPackets() )  {
		log_error(L_GEN, "[vep_cont] packet index %d out of range\n", index);
		return EINVAL;
	}

	nresult = m_arPacket[index]->putData(pData, nSize);
	return nresult;
}

vep_container_head_t* CVepContainer::serialise()
{
	size_t		size, i, count, l, headSize;
	uint8_t		*pBuffer, *p;

	size = getFullSize();

	if ( size <= sizeof(m_inBuffer) )  {
		pBuffer = m_inBuffer;
	}
	else {
		pBuffer = (uint8_t*)::malloc(size);
		if ( pBuffer == NULL ) {
			log_error(L_GEN, "[vep_cont] out of memory!\n");
		}
	}

	if ( pBuffer )  {
		p = pBuffer;
		headSize = getHeadSize();

		UNALIGNED_MEMCPY(p, &m_header, headSize);
		p += headSize;

		count = m_arPacket.size();

		for(i=0; i<count; i++)  {
			vep_packet_head_t*	pPack = *m_arPacket[i];

			l = VEP_PACKET_HEAD_SZ + m_arPacket[i]->getSize();
			UNALIGNED_MEMCPY(p, pPack, l);
			p += l;
		}
	}

	return (vep_container_head_t*)pBuffer;
}

result_t CVepContainer::unserialise(const vep_container_head_t* pHead)
{
	const uint8_t*		p;
	size_t				l, headSize;
	result_t			nresult;

	shell_assert(isEmpty());
	shell_assert(pHead);
	shell_assert(pHead->length > 0);

	headSize = getHeadSize(pHead);
	UNALIGNED_MEMCPY(&m_header, pHead, headSize);

	p = ((const uint8_t*)pHead) + headSize;
	l = 0;
	nresult = ESUCCESS;

	while ( l < pHead->length && nresult == ESUCCESS )  {
		const vep_packet_head_t*	pPack = (const vep_packet_head_t*)p;
		size_t						lpack = pPack->length+VEP_PACKET_HEAD_SZ;

		if ( (pHead->length-l) >= lpack )  {
			nresult = insertPacket(pPack->type, p+VEP_PACKET_HEAD_SZ, lpack-VEP_PACKET_HEAD_SZ);
			if ( nresult == ESUCCESS )  {
				p += lpack;
				l += lpack;
			}
			else {
				nresult = EINVAL;
			}
		}
		else {
			nresult = EINVAL;
			log_debug(L_GEN, "[vep_cont] truncated container, l = %u\n", l);
		}
	}

	if ( nresult == ESUCCESS && l != pHead->length )  {
		log_debug(L_GEN, "[vep_cont] incorrect packet size %u, correct %u\n",
				  pHead->length, l);
		nresult = EINVAL;
	}

	if ( nresult != ESUCCESS )  {
		clear();
	}

	return nresult;
}

void CVepContainer::freeSerialised(vep_container_head_t* pHead)
{
	if ( pHead != NULL && (uint8_t*)pHead != m_inBuffer )  {
		memFree(pHead);
	}
}

/*
 * Finalise a vep container creation, making a valid object
 *
 * Return: ESUCCESS, ...
 */
result_t CVepContainer::finalise()
{
	size_t		i, count;
	result_t	nresult;

	count = getPackets();
	nresult = ESUCCESS;

	for(i=0; i<count; i++)  {
		nresult = m_arPacket[i]->finalise();
		if ( nresult != ESUCCESS )  {
			break;
		}
	}

	if ( nresult == ESUCCESS )  {
		m_header.length = (uint32_t)getDataSize();
	}

	return nresult;
}

/*
 * Send a container
 *
 * 		socket			connected socket
 * 		hrTimeout		maximum send timeout
 *
 * Return: ESUCCESS, ETIMEDOUT, ENOMEM, EINVAL, EINTR
 */
result_t CVepContainer::send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr)
{
	vep_container_head_t*	pHead;
	size_t					fullSize;
	result_t	nresult;

	shell_assert(socket.isOpen());

	nresult = finalise();
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	if ( !isValid() )  {
		return EINVAL;
	}

	pHead = serialise();
	if ( !pHead )  {
		return ENOMEM;
	}

	fullSize = getFullSize();
	pHead->crc = 0;
	pHead->crc = m_header.crc = crc16(pHead, fullSize);

	nresult = socket.send(pHead, fullSize, hrTimeout, dstAddr);
	freeSerialised(pHead);

	return nresult;
}

/*
 * Receive a container with timeout
 *
 * 		socket			connected socket
 * 		hrTimeout		maximum receive time
 *
 * Return: ESUCCESS, ETIMEDOUT, ENOMEM, EINVAL, EINTR
 */
result_t CVepContainer::receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr)
{
	vep_container_head_t*	pHead;
	uint8_t*				p;
	hr_time_t				hrStart;
	size_t					size, headSize;
	result_t				nresult;

	shell_assert(socket.isOpen());
	shell_assert(isEmpty());

	clear();
	pHead = (vep_container_head_t*)&m_inBuffer;

	/*
	 * Read container header
	 */
	hrStart = hr_time_now();
	size = VEP_CONTAINER_HEAD_CONST_SIZE;
	nresult = socket.receive(pHead, &size, CSocket::readFull, hrTimeout, pSrcAddr);
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	p = ((uint8_t*)pHead)+VEP_CONTAINER_HEAD_CONST_SIZE;
	headSize = getHeadSize(pHead);
	if ( headSize > VEP_CONTAINER_HEAD_CONST_SIZE )  {
		size = headSize-VEP_CONTAINER_HEAD_CONST_SIZE;
		nresult = socket.receive(p, &size, CSocket::readFull, hr_timeout(hrStart, hrTimeout));
		if ( nresult != ESUCCESS )  {
			return nresult;
		}

		p += size;
	}

	if ( !checkHeader(pHead) ) {
		log_debug(L_GEN, "[vep_cont] invalid container header received\n");
		return EINVAL;
	}

	/*
	 * Read container data
	 */

	size = pHead->length;
	if ( size > 0 && size <= VEP_CONTAINER_MAX_SIZE )  {
		if ( size > (sizeof(m_inBuffer)-headSize) )  {
			pHead = (vep_container_head_t*)memAlloc(size+headSize);
			if ( pHead != NULL )  {
				UNALIGNED_MEMCPY(pHead, &m_header, headSize);
				p = ((uint8_t*)pHead)+headSize;
			}
			else {
				log_error(L_GEN, "[vep_cont] out of memory\n");
				nresult = ENOMEM;
			}
		}
	}
	else {
		log_error(L_GEN, "[vep_cont] container too large, size %d\n", size);
		nresult = EINVAL;
	}

	if ( nresult != ESUCCESS )  {
		freeSerialised(pHead);
		return nresult;
	}

	nresult = socket.receive(p, &size, CSocket::readFull, hr_timeout(hrStart, hrTimeout));
	if ( nresult == ESUCCESS )  {
		uint16_t	incrc = pHead->crc, crc;

		pHead->crc = 0;
		crc = crc16(pHead, headSize+pHead->length);
		pHead->crc = incrc;
		if ( incrc == crc )  {
			nresult = unserialise(pHead);
		}
		else {
			log_error(L_GEN, "[vep_cont] container CRC wrong (correct %04Xh, real %04Xh), dropped\n",
					  crc, incrc);
			nresult = EFAULT;
		}
	}

	freeSerialised(pHead);

	return nresult;
}

/*******************************************************************************
 * Debugging support
 */

/*
 * Register a new container name
 *
 * 		contType		container type, VEP_CONTAINER_xxx
 * 		strName			container name, static string
 *
 * Return:
 * 		ESUCCESS		successfully registered
 * 		EINVAL			container already registred
 * 		ENOMEM			memory shortage
 */
result_t CVepContainer::registerContainerName(vep_container_type_t contType,
											  const char* strName)
{
	CAutoLock		locker(m_tableNameLock);

	vep_container_string_table_t	table;
	vep_container_names_t::iterator	it;
	result_t						nresult = ESUCCESS;

	it = m_tableName.find(contType);
	if ( it != m_tableName.end() )  {
		log_error(L_GEN, "[vep_cont] container %s(%d) already registred\n",
				  strName, contType);
		return EINVAL;
	}

	table.strName = strName;

	try {
		m_tableName[contType] = table;
	}
	catch(std::bad_alloc& exc) {
		log_error(L_GEN, "[vep_cont] can't allocate container string table\n");
		nresult = ENOMEM;
	}

	return nresult;
}

/*
 * Register a new packet name
 *
 * 		pTable		container string table
 * 		type		packet type
 * 		strName		packet name (static string)
 *
 * Return:
 * 		ESUCCESS		successfully registered
 * 		EINVAL			the packet is already registerd
 * 		ENOMEM			memory shortage
 *
 * Note: container names lock must be held
 */
result_t CVepContainer::registerPacketName(vep_container_string_table_t* pTable,
								   vep_packet_type_t type, const char* strName)
{
	vep_packet_string_table_t			table;
	vep_packet_string_table_t::iterator	it;
	result_t 							nresult = ESUCCESS;

	it = pTable->strTable.find(type);
	if ( it != pTable->strTable.end() )  {
		log_error(L_GEN, "[vep_cont] packet %s(%d) already exists in container %s\n",
				  strName, type, pTable->strName);
		return EINVAL;
	}

	try {
		pTable->strTable[type] = strName;
	}
	catch(std::bad_alloc& exc) {
		log_error(L_GEN, "[vep_cont] can't allocate packet string table\n");
		nresult = ENOMEM;
	}

	return nresult;
}


/*
 * Register array of the packet names
 *
 * 		contType		container type the packets belong to
 * 		nFirst			first packet type
 * 		nLast			last packet type
 * 		strTable		string array in static memory
 *
 * Return:
 * 		ESUCCESS		successfully registered
 * 		EINVAL			parameters are incorrect
 * 		ENOMEM			memory shortage
 */
result_t CVepContainer::registerNames(vep_container_type_t contType,
					vep_packet_type_t nFirst, vep_packet_type_t nLast,
					const char** strTable)
{
	CAutoLock		locker(m_tableNameLock);

	vep_container_names_t::iterator	it;
	vep_container_string_table_t*	pTable;
	vep_packet_type_t				i;
	result_t						nresult = ESUCCESS, nr;

	shell_assert(nFirst >= 0);
	shell_assert(nLast > nFirst);
	shell_assert(strTable);

	it = m_tableName.find(contType);
	if ( it == m_tableName.end() )  {
		log_error(L_GEN, "[vep_cont] container %d is not registred\n", contType);
		return EINVAL;
	}

	pTable = &it->second;

	for(i=nFirst; i <= nLast; i++)  {
		nr = registerPacketName(pTable, i, strTable[i-nFirst]);
		nresult_join(nresult, nr);
	}

	return nresult;
}

/*
 * Get a container name
 *
 * 		type		container type
 * 		strBuf		buffer for a name [out]
 * 		length		buffer length
 */
void CVepContainer::getContainerName(vep_container_type_t type, char* strBuf, size_t length)
{
	CAutoLock		locker(m_tableNameLock);
	vep_container_names_t::const_iterator	it;

	shell_assert(length > 0);

	it = m_tableName.find(type);
	if ( it != m_tableName.end() ) {
		copyString(strBuf, m_tableName[type].strName, length);
	}
	else {
		_tsnprintf(strBuf, length, "%u", type);
	}
}

/*
 * Get a packet name
 *
 * 		contType		container type
 * 		packType		packet type
 * 		strBuf			buffer for a name [out]
 * 		length			buffer length
 *
 */
void CVepContainer::getPacketName(vep_container_type_t contType, vep_packet_type_t packType,
			 					char* strBuf, size_t length)
{
	CAutoLock 	locker(m_tableNameLock);
	vep_container_names_t::const_iterator	it;

	shell_assert(length > 0);

	it = m_tableName.find(contType);
	if ( it != m_tableName.end() ) {
		vep_packet_string_table_t::const_iterator	itp;

		itp = it->second.strTable.find(packType);
		if ( itp != it->second.strTable.end() )  {
			copyString(strBuf, itp->second, length);
		}
		else {
			_tsnprintf(strBuf, length, "packet(%u)", packType);
		}
	}
	else {
		copyString(strBuf, "UNKNOWN", length);
	}
}

void CVepContainer::dumpNames(const char* strPref)
{
	CAutoLock 	locker(m_tableNameLock);
	vep_container_names_t::const_iterator		it;
	vep_packet_string_table_t::const_iterator	itp;
	const vep_container_string_table_t*			pTable;

	log_dump("%sVepContainer names (%u containers)\n",
			 strPref ? strPref : "", m_tableName.size());

	for(it = m_tableName.begin(); it != m_tableName.end(); it++)  {
		pTable = &it->second;

		log_dump("  => Container:  VEP_CONTAINER_%s (%u)  (%u packets)\n",
				 pTable->strName, (unsigned)it->first, pTable->strTable.size());

		for(itp = pTable->strTable.begin(); itp != pTable->strTable.end(); itp++)  {
			log_dump("     | packet: %-2u %s\n", itp->first, itp->second);
		}
	}
}

/*
 * Dump container/packets
 *
 * 		strPref		dump prefix, may be 0
 */
void CVepContainer::dump(const char* strPref) const
{
	char	strIdent[32], strTmp[128];
	size_t	lenIdent = _tstrlen((const char*)VEP_IDENT), count, i;

	UNALIGNED_MEMCPY(strIdent, &m_header.ident, lenIdent);
	strIdent[lenIdent] = 0;

	CVepContainer::getContainerName(m_header.type, strTmp, sizeof(strTmp));
	count = getPackets();

	log_dump("| %sVep-Container: ident \"%s\" (%s), version: %u, type: %s (%d), flags: %02Xh\n",
			 strPref, strIdent,
			 _tmemcmp(strIdent, VEP_IDENT, lenIdent) == 0 ? "valid" : "INVALID",
			 m_header.version, strTmp, m_header.type, m_header.flags);


	_tsnprintf(strTmp, sizeof(strTmp), "| %sVep-Container: size: h: %u, d: %u, "
				"crc: %04Xh, packets: %u, src_addr %Xh, dst_addr %Xh\n",
				strPref, (unsigned)getHeadSize(), (unsigned)m_header.length,
				m_header.crc, (unsigned)count,
				(m_header.flags&VEP_CONT_ADDR) ? m_header.src : 0,
				(m_header.flags&VEP_CONT_ADDR) ? m_header.dst : 0);

	log_dump(strTmp);

	for(i=0; i<count; i++) {
		m_arPacket[i]->dump(m_header.type, strPref);
	}
}

void CVepContainer::getDump(char* strBuf, size_t length) const
{
	size_t	len;

	getContainerName(getType(), strBuf, length);
	len = _tstrlen(strBuf);
	if ( (len+2) < length && getPackets() > 0 )  {
		strBuf[len] = '/';
		getPacketName(getType(), m_arPacket[0]->getType(), &strBuf[len+1], length-len-1);
	}
}
