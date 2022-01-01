/*
 *  Carbon framework
 *  TCP server base class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 06.05.2015 23:47:40
 *      Initial revision.
 */

#ifndef __CARBON_TCP_SERVER_H_INCLUDED__
#define __CARBON_TCP_SERVER_H_INCLUDED__

#include "shell/socket.h"
#include "shell/atomic.h"
#include "shell/object.h"

#include "carbon/cstring.h"

class CTcpServer : public CObject
{
	protected:
		CSocketRef*			m_pSocket;			/* Using socket object */
		CNetAddr			m_servAddr;		/* Address to listen on */
		CString				m_strSocket;		/* Socket to listen on */
		atomic_t			m_nStop;			/* TRUE: stopping server */

	public:
		explicit CTcpServer(const char* strName);
		virtual ~CTcpServer();

	public:
		virtual result_t run();
		virtual void stop() { sh_atomic_inc(&m_nStop); }

	public:
		boolean_t isAddrLocal() const {
			return !m_strSocket.isEmpty();
		}

		boolean_t isAddrValid() const {
			return !isAddrLocal() ? m_servAddr.isValid() : TRUE;
		}
		const char* servAddrStr() const {
			return isAddrLocal() ? m_strSocket.cs() : m_servAddr.cs();
		}

		virtual void setAddr(const CNetAddr& servAddr)  {
			m_servAddr = servAddr;
			m_strSocket.clear();
		}

		virtual void setAddr(const char* strSocket) {
			m_strSocket = strSocket;
		}

		virtual void getAddr(CNetAddr& servAddr) const {
			servAddr = m_servAddr;
		}

		virtual void getAddr(CString& strSocket) const {
			strSocket = m_strSocket;
		}

	protected:
		virtual boolean_t isStopping() const {
			return sh_atomic_get(&m_nStop) != 0;
		}

		virtual result_t processClient(CSocketRef* pSocket) = 0;
};

#endif /* __CARBON_TCP_SERVER_H_INCLUDED__ */
