/*
 *  Carbon framework
 *  Execute shell command
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.08.2016 16:48:34
 *      Initial revision.
 */

#ifndef __CARBON_SHELL_EXECUTE_CLIENT_H_INCLUDED__
#define __CARBON_SHELL_EXECUTE_CLIENT_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/event.h"

typedef struct {
	int 	retVal;
	char*	strOutBuffer;
	size_t	szOutBuffer;
} shell_execute_t;

extern result_t shellExecute(const char* strCmd, CEventReceiver* pReplyReceiver,
							 seqnum_t nSessId);
extern result_t shellExecuteSync(const char* strCmd, CEventReceiver* pReplyReceiver,
								 hr_time_t hrTimeout, shell_execute_t* pOutData);

extern result_t shellExecuteStartServer(const char* strExecPath);
extern result_t shellExecuteStopServer();

#endif /* __CARBON_SHELL_EXECUTE_CLIENT_H_INCLUDED__ */
