/*
 *  Carbon framework
 *  Simple raw container
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 09.09.2016 12:57:12
 *      Initial revision.
 */

#ifndef __CARBON_RAW_CONTAINER_H_INCLUDED__
#define __CARBON_RAW_CONTAINER_H_INCLUDED__

#include "shell/socket.h"

#include "carbon/net_container.h"

class CRawContainer : public CNetContainer
{
	protected:
		uint8_t*		m_pBuffer;
		size_t			m_nCurSize;
		size_t			m_nSize;
		const size_t	m_nMaxSize;

	public:
		CRawContainer(size_t nMaxSize);
	protected:
		virtual ~CRawContainer();

	public:
		operator uint8_t*()	{ return m_pBuffer; }

		virtual void* getData() { return m_pBuffer; }
		virtual size_t getSize() const { return m_nCurSize; }

		virtual void clear();
		virtual CNetContainer* clone();

		virtual result_t send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr = NETADDR_NULL);
		virtual result_t receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr = NULL);

		virtual result_t putData(const void* pData, size_t nSize);

		virtual void dump(const char* strPref = "") const;
		virtual void getDump(char* strBuf, size_t length) const;

	protected:
		result_t makeBuffer(size_t nSize);

};

#endif /* __CARBON_RAW_CONTAINER_H_INCLUDED__ */
