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
 *  Revision 1.0, 22.03.2015 23:38:43
 *      Initial revision.
 */
/*
 * VEP protocol container:
 *
 * 	1) All multi-byte data in LSB;
 *
 */

#ifndef __CARBON_VEP_CONTAINER_H_INCLUDED__
#define __CARBON_VEP_CONTAINER_H_INCLUDED__

#include <map>
#include <vector>

#include "shell/socket.h"
#include "shell/hr_time.h"
#include "shell/error.h"

#include "carbon/net_container.h"
#include "carbon/lock.h"
#include "vep/vep.h"

/*
 * Vep container/packet names
 */

typedef std::map<vep_packet_type_t, const char*>	vep_packet_string_table_t;

typedef struct {
	const char*					strName;
	vep_packet_string_table_t 	strTable;
} vep_container_string_table_t;

typedef std::map<vep_container_type_t, vep_container_string_table_t>	vep_container_names_t;


/*
 * Container types
 */
static const vep_container_type_t VEP_CONTAINER_SYSTEM = 0;
static const vep_container_type_t VEP_CONTAINER_APP = 1;

/*
 * Container source/destination addresses
 */
typedef uint16_t	vep_addr_t;

static const vep_addr_t VEP_ADDR_NONE = 0;
static const vep_addr_t VEP_ADDR_BROADCAST = 0xffff;


/*
 * Container flags
 */
#define VEP_CONT_CRYPT				0x00000001		/* Data is encrypted AES */
#define VEP_CONT_COMPRESS			0x00000002		/* Data is gzip compressed */
#define VEP_CONT_PACKED				0x00000004		/* Data is packed */
#define VEP_CONT_ADDR				0x00000008		/* Src/Dst addresses are present */

#define VEP_CONT_RESERVED_SHIFT		4
#define VEP_CONT_RESERVED_MASK		0xf
#define VEP_CONT_RESERVED1			0x00000010
#define VEP_CONT_RESERVED2			0x00000020
#define VEP_CONT_RESERVED3			0x00000040
#define VEP_CONT_RESERVED4			0x00000080

typedef struct {
	uint8_t					ident[4];		/* Identification string "veri" */
	uint32_t				version;		/* Version number */
	vep_container_type_t	type;			/* Container type, VEP_CONTAINER_xxx */
	uint32_t				flags;			/* Flags */
	uint32_t				length;			/* Full container length EXCLUDING header, bytes */
	uint16_t				crc;			/* Checksum CRC16 */

	vep_addr_t				src;			/* Source address (sent from) (optional) */
	vep_addr_t				dst;			/* Destination address (sent to) optional) */

	uint32_t				reserved1;
	uint32_t				reserved2;
	uint32_t				reserved3;
	uint32_t				reserved4;
} __attribute__ ((packed)) vep_container_head_t;

#define VEP_CONTAINER_HEAD_CONST_SIZE	\
	(sizeof(vep_container_head_t)-sizeof(vep_addr_t)*2-sizeof(uint32_t)*4)


class CVepContainer : public CNetContainer
{
	protected:
		vep_container_head_t		m_header;				/* Container header */
		std::vector<CVepPacket*>	m_arPacket;				/* Packets array */

		uint8_t						m_inBuffer[PAGE_SIZE];	/* Temporary internal buffer */

		static vep_container_names_t	m_tableName;
		static CMutex					m_tableNameLock;

	public:
		CVepContainer();
		CVepContainer(vep_container_type_t contType, vep_packet_type_t packType = VEP_PACKET_TYPE_NULL);
	protected:
		virtual ~CVepContainer();

	public:
		CVepPacket* operator[](size_t index) {
			shell_assert(index < m_arPacket.size());
			return index < m_arPacket.size() ? m_arPacket[index] : NULL;
		}

		vep_packet_head_t* getPacketHead(size_t index = 0)  {
			shell_assert(index < m_arPacket.size());
			return index < m_arPacket.size() ? ((vep_packet_head_t*)(*m_arPacket[index])) : 0;
		}

		virtual result_t create(vep_container_type_t contType, vep_packet_type_t packType = VEP_PACKET_TYPE_NULL);
		virtual void clear();
		virtual CNetContainer* clone();

		virtual boolean_t isValid() const;
		virtual boolean_t isEmpty() const { return m_arPacket.empty(); }
		vep_container_type_t getType() const { return m_header.type; }
		size_t getPackets() const { return m_arPacket.size(); }
		virtual result_t finalise();

		int getPacketType(size_t index = 0) const {
			shell_assert(index < m_arPacket.size());
			return m_arPacket[index]->getType();
		}

		virtual result_t insertPacket(vep_packet_type_t packType);
		virtual result_t insertPacket(vep_packet_type_t packType, const void* pData, size_t nSize);
		virtual result_t insertData(const void* pData, size_t nSize, size_t index = 0);

		virtual result_t send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr = NETADDR_NULL);
		virtual result_t receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr = NULL);

		/* VEP string table management */

		virtual void dump(const char* strPref = "") const;
		virtual void getDump(char* strBuf, size_t length) const;

		/* Container/packet names */
		static result_t registerContainerName(vep_container_type_t contType, const char* strName);
		static result_t registerNames(vep_container_type_t contType,
							vep_packet_type_t nFirst, vep_packet_type_t nLast,
							const char** strTable);

		static void getContainerName(vep_container_type_t type, char* strBuf, size_t length);
		static void getPacketName(vep_container_type_t contType, vep_packet_type_t packType,
				 		char* strBuf, size_t length);

		static void dumpNames(const char* strPref = 0);

	protected:
		boolean_t checkHeader(const vep_container_head_t* pHead) const;
		size_t getHeadSize(const vep_container_head_t* pHead = 0) const;
		size_t getDataSize() const;
		size_t getFullSize() const { return getHeadSize()+getDataSize(); }

		vep_container_head_t* serialise();
		result_t unserialise(const vep_container_head_t* pHead);
		void freeSerialised(vep_container_head_t* pBuffer);

	private:
		void deletePacket(size_t index);

		static result_t registerPacketName(vep_container_string_table_t* pTable,
						vep_packet_type_t type, const char* strName);
};


#endif /* __CARBON_VEP_CONTAINER_H_INCLUDED__ */
