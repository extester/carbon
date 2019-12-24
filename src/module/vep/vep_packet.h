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
 *  Revision 1.0, 04.05.2015 00:21:52
 *      Initial revision.
 */

#ifndef __CARBON_VEP_PACKET_H_INCLUDED__
#define __CARBON_VEP_PACKET_H_INCLUDED__

#include "shell/shell.h"
#include "shell/socket.h"

#include "vep/vep.h"

typedef uint32_t 				vep_packet_type_t;

#define VEP_PACKET_TYPE_NULL			((vep_packet_type_t)0)
#define VEP_PACKET_INSIZE				PAGE_SIZE
#define VEP_PACKET_HEAD_SZ				sizeof(vep_packet_head_t)

/*
 * Common header for all packets
 */
typedef struct {
	vep_packet_type_t	type;			/* Packet type */
	uint32_t			length;			/* Length of the data after header, bytes */
} __attribute__ ((packed)) vep_packet_head_t;


class CVepPacket
{
	protected:
		uint8_t				m_inBuffer[VEP_PACKET_INSIZE];	/* Inline buffer */
		uint8_t*			m_pPack;		/* Current buffer pointer */
		size_t				m_nSize;		/* Current maximum data size, bytes */

	public:
		CVepPacket();
		CVepPacket(vep_packet_type_t type);
		CVepPacket(const CVepPacket& packet);
		virtual ~CVepPacket();

	public:
		operator vep_packet_head_t*() {
			return (vep_packet_head_t*)m_pPack;
		}

		vep_packet_head_t* getHead() {
			return (vep_packet_head_t*)m_pPack;
		}

		virtual boolean_t isValid() const;

		virtual void create(vep_packet_type_t type);
		virtual void clear();

		virtual result_t putData(const void* pData, size_t nSize);
		virtual result_t finalise();

		virtual vep_packet_type_t getType() const { return ((vep_packet_head_t*)m_pPack)->type; }
		virtual uint32_t getSize() const {
			return (uint32_t)VEP_PACKET_HEAD_SZ+((vep_packet_head_t*)m_pPack)->length;
		}

		virtual void dumpData(const char* strPref = "") const;
		virtual void dump(vep_container_type_t contType, const char* strPref = "") const;

	private:
		result_t expandBuffer(size_t nSize);
		boolean_t isInline() const { return m_pPack == m_inBuffer; }
};

#endif /* __CARBON_VEP_PACKET_H_INCLUDED__ */
