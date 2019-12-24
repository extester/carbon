/*
 *  Carbon framework
 *  Network Packet exchanger class
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 01.08.2016 14:41:06
 *      Initial revision.
 */

#ifndef __CARBON_PACKET_IO_H_INCLUDED__
#define __CARBON_PACKET_IO_H_INCLUDED__

#include "shell/hr_time.h"
#include "shell/netaddr.h"

#include "carbon/net_container.h"

class CPacketIo
{
	protected:
		hr_time_t		m_hrTimeout;

	public:
		CPacketIo(hr_time_t hrTimeout) : m_hrTimeout(hrTimeout) {
		}
		virtual ~CPacketIo() {
		}

	public:
		result_t execute(CNetContainer* pInContainer, CNetContainer* pOutContainer,
						 const CNetAddr& dstAddr, const CNetAddr& srcAddr = NETADDR_NULL);
};

#endif /* __CARBON_PACKET_IO_H_INCLUDED__ */
