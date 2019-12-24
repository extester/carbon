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
 *  Revision 1.0, 01.06.2015 12:07:51
 *      Initial revision.
 */

#include "carbon/carbon.h"
#include "vep/vep_server_thread.h"


CVepServerThread::CVepServerThread(const CNetAddr& netAddr) :
	CVepServer(),
	m_thServer("VepServerThread")
{
	setListenAddr(netAddr);
}

CVepServerThread::~CVepServerThread()
{
	shell_assert(!m_thServer.isRunning());
}

void* CVepServerThread::thread(CThread* pThread, void* pData)
{
	pThread->bootCompleted(ESUCCESS);

	log_debug(L_BOOT, "[vep_server] starting server thread, listen on %s\n",
			  (const char*)m_listenAddr);
	run();
	log_debug(L_BOOT, "[vep_server] server thread has been stopped\n");

	return NULL;
}


/*
 * Start server thread
 *
 * Return: ESUCCESS, ...
 */
result_t CVepServerThread::start()
{
	result_t	nresult;

	nresult = m_thServer.start(THREAD_CALLBACK(CVepServerThread::thread, this));
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[vep_server] failed to start server, result: %d\n", nresult);
	}

	return nresult;
}

/*
 * Stop server thread
 */
void CVepServerThread::stop()
{
	CTcpServer::stop();
	m_thServer.stop();
}
