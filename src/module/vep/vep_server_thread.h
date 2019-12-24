/*
 *  Carbon/Vep module
 *  Info-packet server thread
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 01.06.2015 12:01:48
 *      Initial revision.
 */

#ifndef __CARBON_VEP_SERVER_THREAD_H_INCLUDED__
#define __CARBON_VEP_SERVER_THREAD_H_INCLUDED__

#include "carbon/thread.h"
#include "vep/vep_server.h"


class CVepServerThread : public CVepServer
{
	private:
		CThread			m_thServer;

	public:
		CVepServerThread(const CNetAddr& netAddr = NETADDR_NULL);
		virtual ~CVepServerThread();

	public:
		virtual result_t start();
		virtual void stop();

	private:
		void* thread(CThread* pThread, void* pData);
};

#endif /* __CARBON_VEP_SERVER_THREAD_H_INCLUDED__ */
