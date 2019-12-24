/*
 *  Carbon/Vep module
 *  Verinet Exchange Protocol (VEP) server
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 19.05.2015 02:21:25
 *      Initial revision.
 */

#ifndef __CARBON_VEP_SERVER_H_INCLUDED__
#define __CARBON_VEP_SERVER_H_INCLUDED__

#include "shell/shell.h"
#include "shell/socket.h"

#include "carbon/tcp_server.h"
#include "vep/vep.h"

class CVepServer : public CTcpServer
{
	protected:
		hr_time_t		m_hrRecvTimeout;

	public:
		CVepServer();
		virtual ~CVepServer();

	public:
		void setRecvTimeout(hr_time_t hrRecvTimeout) {
			m_hrRecvTimeout = hrRecvTimeout;
		}

	protected:
		virtual result_t processClient(CSocketRef* pSocket);
		virtual void processPacket(CSocketRef* pSocket, CVepContainer* pContainer) = 0;
};

#endif /* __CARBON_VEP_SERVER_H_INCLUDED__ */
