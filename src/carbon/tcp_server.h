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
		CNetAddr			m_listenAddr;		/* Address to listen on */
		CString				m_strSocket;		/* Socket to listen on */
		atomic_t			m_nStop;			/* TRUE: stopping server */

	public:
		CTcpServer(const char* strName);
		virtual ~CTcpServer();

	public:
		virtual result_t run();
		virtual void stop() { atomic_inc(&m_nStop); }

	protected:
		virtual void setListenAddr(const CNetAddr& listenAddr)  {
			m_listenAddr = listenAddr;
		}

		virtual void setListenAddr(const char* strSocket) {
			m_strSocket = strSocket;
		}

		virtual void getListenAddr(CNetAddr& listenAddr) const {
			listenAddr = m_listenAddr;
		}

		virtual void getListenAddr(CString& strSocket) const {
			strSocket = m_strSocket;
		}

		virtual boolean_t isStopping() const {
			return atomic_get(&m_nStop) != 0;
		}

		virtual result_t processClient(CSocketRef* pSocket) = 0;
};

#endif /* __CARBON_TCP_SERVER_H_INCLUDED__ */
