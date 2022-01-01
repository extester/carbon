/*
 *	Carbon Framework Examples
 *	Execute external program
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 05.08.2016 17:18:53
 *	    Initial revision.
 */
/*
 * Execute external programs
 *  a) asynchronous
 *  b) synchronous
 */

#include "carbon/shell_execute.h"
#include "carbon/event/remote_event_service.h"

#include "shell_execute_app.h"

#define SHELL_EXECUTE_EXAMPLE_RID       "carbon.example.shell_execute"
#define SHELL_EXECUTE_PROGRAM_ASYNC     "ls -al /*.sh"
#define SHELL_EXECUTE_PROGRAM_SYNC      "ls -al /*.*"


/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CShellExecuteApp::CShellExecuteApp(int argc, char* argv[]) :
    CApplication("Shell execute Application", MAKE_VERSION(1,0,0), 1, argc, argv),
    m_sessId(NO_SEQNUM)
{
}

/*
 * Application class destructor
 */
CShellExecuteApp::~CShellExecuteApp()
{
}

/*
 * Dump remote event
 *
 * 		pEvent		remote event to dump parameters
 */
void CShellExecuteApp::dumpEvent(CRemoteEvent* pEvent) const
{
    CString         strPayload;
    const char*     strData = (const char*)pEvent->getData();
    size_t          size = pEvent->getDataSize();

    pEvent->dump();

    if ( strData != 0 && size > 0 )  {
        size = sh_min(size, 16536);
        strPayload.append(strData, size);
        log_dump("\t\t ool data:\n'%s'\n", (const char*)strPayload);
    }
}

/*
 * Event processor
 *
 *      pEvent      event object to process
 *
 * Return:
 *      TRUE        event processed
 *      FALSE       event is not processed
 */
boolean_t CShellExecuteApp::processEvent(CEvent* pEvent)
{
    CRemoteEvent*   pRemoteEvent;
    boolean_t       bProcessed;

    switch ( pEvent->getType() )  {
        case EV_R_SHELL_EXECUTE_REPLY:
            pRemoteEvent = dynamic_cast<CRemoteEvent*>(pEvent);
            shell_assert(pRemoteEvent);
            if ( pRemoteEvent && m_sessId == pRemoteEvent->getSessId() ) {
                log_info(L_GEN, "[app] received asynchronous result %d, program retcode: %d\n",
                               (result_t)pEvent->getnParam(), (int)(natural_t)pEvent->getpParam());
                dumpEvent(pRemoteEvent);
                m_sessId = NO_SEQNUM;

                /*
    			 * Execute external program Asynchronous
		    	 */
                log_info(L_GEN, "[app] RUNNING external program '%s' SYNCHRONOUS...\n",
                             SHELL_EXECUTE_PROGRAM_SYNC);

                char            buf[1024];
                shell_execute_t data = { 0, buf, sizeof(buf) };
                result_t        nresult;

                nresult = shellExecuteSync(SHELL_EXECUTE_PROGRAM_SYNC, this, HR_3SEC, &data);
                log_info(L_GEN, "[app] synchronous result %d, program retcode %d, output:\n'%s'\n",
                             nresult, data.retVal, buf);
                stopApplication(0);
            }

            bProcessed = TRUE;
            break;

        default:
            bProcessed = CApplication::processEvent(pEvent);
            break;
    }

    return bProcessed;
}


/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CShellExecuteApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* The Remote Event Service is not initialised automatically */
    nresult = initRemoteEventService(SHELL_EXECUTE_EXAMPLE_RID, this);
    if ( nresult == ESUCCESS ) {
        /*
         * Execute external program Asynchronous
         * The program result is processing by EV_R_SHELL_EXECUTE_REPLY
         */
        log_info(L_GEN, "[app] RUNNING external program '%s' asynchronous...\n",
                 SHELL_EXECUTE_PROGRAM_ASYNC);

        m_sessId = getUniqueId();
        nresult = shellExecute(SHELL_EXECUTE_PROGRAM_ASYNC, this, m_sessId);
        if ( nresult != ESUCCESS )  {
            log_error(L_GEN, "[app] can't execute '%s', exiting...\n",
                      SHELL_EXECUTE_PROGRAM_ASYNC);
        }
    }

    if ( nresult != ESUCCESS )  {
        CApplication::terminate();
    }

    return nresult;
}

/*
 * Application termination
 */
void CShellExecuteApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
    exitRemoteEventService();

	CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    return CShellExecuteApp(argc, argv).run();
}
