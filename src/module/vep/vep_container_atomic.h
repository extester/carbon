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
 *  Revision 1.0, 27.09.2016 18:28:10
 *      Initial revision.
 */
/*
 * Vep container for UDP I/O.
 * Used single recv/send calls for I/O
 */

#ifndef __CARBON_VEP_CONTAINER_ATOMIC_H_INCLUDED__
#define __CARBON_VEP_CONTAINER_ATOMIC_H_INCLUDED__

#include "vep/vep_container.h"

class CVepContainerAtomic : public CVepContainer
{
	protected:
		size_t		m_nMaxSize;

	public:
		CVepContainerAtomic(size_t nMaxSize = 2048);
		CVepContainerAtomic(vep_container_type_t contType,
							vep_packet_type_t packType = VEP_PACKET_TYPE_NULL,
							size_t nMaxSize = 2048);
	protected:
		virtual ~CVepContainerAtomic();

	public:
		virtual CNetContainer* clone();

		virtual result_t send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr = NETADDR_NULL) {
			return CVepContainer::send(socket, hrTimeout, dstAddr);
		}
		virtual result_t receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr = NULL);

		virtual void dump(const char* strPref = "") const {
			CVepContainer::dump(strPref);
		}
		virtual void getDump(char* strBuf, size_t length) const {
			CVepContainer::getDump(strBuf, length);
		}
};


#endif /* __CARBON_VEP_CONTAINER_ATOMIC_H_INCLUDED__ */
