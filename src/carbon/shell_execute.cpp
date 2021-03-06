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
 *  Revision 1.0, 05.08.2016 16:54:29
 *      Initial revision.
 */

#include "carbon/carbon.h"
#include "carbon/event/remote_event_service.h"
#include "carbon/sync.h"
#include "carbon/shell_execute.h"

#include "contact/proc_utils.h"

class CSyncShellExecute : public CSyncBase
{
	protected:
		shell_execute_t*	m_pOutData;

	public:
		CSyncShellExecute(seqnum_t sessId, shell_execute_t* pOutData) :
			CSyncBase(sessId),
			m_pOutData(pOutData)
		{
		}

		virtual ~CSyncShellExecute()
		{
		}

	public:
		virtual result_t processSyncEvent();
};

result_t CSyncShellExecute::processSyncEvent()
{
	CRemoteEvent*	pEvent;
	const void*		strData;
	size_t			size;
	result_t		nrChild;

	if ( m_pOutData != NULL && m_pOutData->szOutBuffer > 0 )  {
		*m_pOutData->strOutBuffer = '\0';
	}

	if ( (CEvent*)m_pEvent == 0 )  {
		if ( m_pOutData ) { m_pOutData->retVal = ETIMEDOUT; }
		return ETIMEDOUT;
	}

	pEvent = dynamic_cast<CRemoteEvent*>((CEvent*)m_pEvent);
	shell_assert(pEvent);
	if ( !pEvent )  {
		if ( m_pOutData ) { m_pOutData->retVal = EINVAL; }
		return EINVAL;
	}

	nrChild = (result_t)pEvent->getnParam();
	if ( nrChild != ESUCCESS )  {
		/*
		 * The operation failed, event ignored
		 */
		if ( m_pOutData ) { m_pOutData->retVal = nrChild; }
		return nrChild;
	}

	strData = pEvent->getData();
	size = pEvent->getDataSize();

	if ( strData != NULL && size > 0 && m_pOutData != NULL && m_pOutData->szOutBuffer > 0 )  {
		UNALIGNED_MEMCPY(m_pOutData->strOutBuffer, strData, m_pOutData->szOutBuffer);
	}

	if ( m_pOutData )  { m_pOutData->retVal = (int)(natural_t)pEvent->getpParam(); }
	return nrChild;
}

result_t shellExecute(const char* strCmd, CEventReceiver* pReplyReceiver, seqnum_t nSessId)
{
	CRemoteEvent*	pEvent;
	result_t		nresult;

	if ( strCmd == 0 || *strCmd == '\0' || !pReplyReceiver || nSessId == NO_SEQNUM )  {
		return EINVAL;
	}

	pEvent = new CRemoteEvent(EV_R_SHELL_EXECUTE, 0, pReplyReceiver, nSessId,
							  strCmd, _tstrlen(strCmd));
	nresult = appSendRemoteEvent(pEvent, CARBON_SHELL_EXECUTE_RID, pReplyReceiver, nSessId);

	return nresult;
}

result_t shellExecuteSync(const char* strCmd, CEventReceiver* pReplyReceiver,
						  hr_time_t hrTimeout, shell_execute_t* pOutData)
{
	seqnum_t			sessId = getUniqueId();
	CSyncShellExecute	syncer(sessId, pOutData);
	CRemoteEvent*		pEvent;
	CEventLoop*			pEventLoop;
	result_t			nresult;

	shell_assert(pReplyReceiver);
	shell_assert(strCmd);

	if ( strCmd == 0 || *strCmd == '\0' || !pReplyReceiver )  {
		return EINVAL;
	}

	pEventLoop = pReplyReceiver->getEventLoop();
	if ( pEventLoop ) {
		pEventLoop->attachSync(&syncer);

		pEvent  = new CRemoteEvent(EV_R_SHELL_EXECUTE, 0, pReplyReceiver, sessId,
								   strCmd, _tstrlen(strCmd));
		nresult = appSendRemoteEvent(pEvent, CARBON_SHELL_EXECUTE_RID);
		if ( nresult == ESUCCESS) {
			nresult = pEventLoop->waitSync(hrTimeout);
		}
		else {
			log_error(L_GEN, "[shell_execute] failed to send event, result: %d\n", nresult);
		}

		pEventLoop->detachSync();
	}
	else {
		log_error(L_GEN, "[shell_execute] event loop is not found\n");
		nresult = ESRCH;
	}

	return nresult;
}

#define SHELL_EXECUTE_PROGRAM			"shell_execute"

result_t shellExecuteStartServer(const char* strExecPath)
{
	char			strPath[256];
	CProcUtils		pu;
	proc_data_t		pu_data;
	size_t			count;
	result_t		nresult;

	pid_t			pid;
	static const char* argv[] = { SHELL_EXECUTE_PROGRAM, "", 0 };

	/*
	 * Check if a shell_execute server is running already
	 */
	count = pu.getByName(SHELL_EXECUTE_PROGRAM, &pu_data, 1);
	if ( count > 0 )  {
		//log_debug(L_GEN, "[shell_execute] server already started, count=%d\n", count);
		return EEXIST;
	}

	copyString(strPath, strExecPath, sizeof(strPath));
	appendPath(strPath, SHELL_EXECUTE_PROGRAM, sizeof(strPath));

	pid = fork();
	if ( pid == -1 )  {
		nresult = errno;
		log_error(L_GEN, "[shell_execute] fork() failed, result %d\n", nresult);
		return nresult;
	}

	if ( pid != 0 )  {
		return ESUCCESS;
	}
	/*
	 * Child process
	 */

	execv(strPath, (char* const*)argv);
	nresult = errno;
	log_error(L_GEN, "[shell_execute] execv() failed, path %s, result %s(%d)\n",
			  	strPath, strerror(nresult), nresult);

	exit(127); /* command not found */
	return nresult;
}

result_t shellExecuteStopServer()
{
	CProcUtils	pu;
	size_t		count;
	result_t	nresult;

	count = pu.killByName(SHELL_EXECUTE_PROGRAM, HR_30SEC);
	nresult = count > 0 ? ESUCCESS : ENOENT;

	return nresult;
}
