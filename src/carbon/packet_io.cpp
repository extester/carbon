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
 *  Revision 1.0, 01.08.2016 14:42:06
 *      Initial revision.
 */

#include "shell/socket.h"

#include "carbon/logger.h"
#include "carbon/packet_io.h"


result_t CPacketIo::execute(CNetContainer* pInContainer, CNetContainer* pOutContainer,
							const CNetAddr& dstAddr, const CNetAddr& srcAddr)
{
	CNetContainer*		pOut;
	CSocket				ioSocket;
	hr_time_t			hrStart, hrTimeout;
	result_t			nresult;

	hrStart = hr_time_now();
	nresult = ioSocket.connect(dstAddr, m_hrTimeout, srcAddr);
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[vep_packet_io] failed to connect to %s in %d secs, result: %d\n",
						(const char*)dstAddr, HR_TIME_TO_SECONDS(m_hrTimeout), nresult);
		return nresult;
	}

	hrTimeout = hr_timeout(hrStart, m_hrTimeout);
	nresult = pInContainer->send(ioSocket, hrTimeout);
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[vep_packet_io] failed to send packet to %s, result: %d\n",
						(const char*)dstAddr, nresult);
		ioSocket.close();
		return nresult;
	}

	if ( pOutContainer )  {
		pOutContainer->clear();
		pOut = pOutContainer;
	}
	else {
		pInContainer->clear();
		pOut = pInContainer;
	}

	hrTimeout = hr_timeout(hrStart, m_hrTimeout);
	nresult = pOut->receive(ioSocket, hrTimeout);
	ioSocket.close();
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[vep_packet_io] failed to receive reply from %s, result: %d\n",
						(const char*)dstAddr, nresult);
	}

	return nresult;
}
