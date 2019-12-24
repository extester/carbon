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
 *  Revision 1.0, 19.05.2015, 21:22:52
 *      Initial revision.
 */

#include "shell/hr_time.h"
#include "shell/dec_ptr.h"

#include "carbon/logger.h"
#include "vep/vep_server.h"

#define VEP_SERVER_TIMEOUT_DEFAULT			HR_10SEC

/*******************************************************************************
 * CVepServer class
 */

CVepServer::CVepServer() :
	CTcpServer("VepServer"),
	m_hrRecvTimeout(VEP_SERVER_TIMEOUT_DEFAULT)
{
}

CVepServer::~CVepServer()
{
}

result_t CVepServer::processClient(CSocketRef* pSocket)
{
	dec_ptr<CVepContainer>	containerPtr = new CVepContainer();
	result_t				nresult;
	boolean_t				bResult;

	shell_assert(pSocket->isOpen());

	nresult = containerPtr->receive(*pSocket, m_hrRecvTimeout);
	if ( nresult == ESUCCESS ) {
		bResult = containerPtr->isValid();
		if ( bResult ) {
			/*
			 * Received a valid info-container
			 */
			processPacket(pSocket, containerPtr);
		}
		else {
			log_error(L_GEN, "[vep_server] received invalid container, dropped\n");
		}
	}
	else {
		log_error(L_GEN, "[info_server] failed to receive an info-container, result: %d\n", nresult);
	}

	return nresult;
}
