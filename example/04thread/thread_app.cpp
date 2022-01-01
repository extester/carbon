/*
 *	Carbon Framework Examples
 *	Worker thread
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 01.06.2016 12:49:15
 *	    Initial revision.
 */

#include <unistd.h>

#include "shell/hr_time.h"

#include "thread_app.h"

#define EVENT_INTERVAL          HR_2SEC

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CThreadApp::CThreadApp(int argc, char* argv[]) :
    CApplication("Thread Application", MAKE_VERSION(1,0,0), 1, argc, argv),
    m_thWorker("worker")
{
}

/*
 * Application class destructor
 */
CThreadApp::~CThreadApp()
{
    shell_assert(!m_thWorker.isRunning());
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
boolean_t CThreadApp::processEvent(CEvent* pEvent)
{
    boolean_t   bProcessed;

    switch ( pEvent->getType() )  {
        case EV_TEST:
            log_info(L_GEN, "*** Received Test event [0x%X, 0x%X] ***\n",
                     pEvent->getpParam(), pEvent->getnParam());
            bProcessed = TRUE;
            break;

        default:
            bProcessed = CApplication::processEvent(pEvent);
            break;
    }

    return bProcessed;
}

/*
 * Worker thread routine
 *
 *    pThread       thread object
 *    pData         transparent data pointer passed to CTimer::start()
 *
 * Return: thread exit code
 */
void* CThreadApp::threadProc(CThread* pThread, void* pData)
{
    CEvent*     pEvent;
    static int  n = 0;

    pThread->bootCompleted(ESUCCESS);

    while ( !pThread->isStopping() ) {
        log_info(L_GEN, "thread: *** sending Test event ***\n");

        pEvent = new CEvent(EV_TEST, this, (PPARAM)0x1122, (NPARAM)n++, "Test event");
        appSendEvent(pEvent);

        sleep_us(HR_TIME_TO_MICROSECONDS(EVENT_INTERVAL));
    }

    return NULL;
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CThreadApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* User application inialisation */
    nresult = m_thWorker.start(THREAD_CALLBACK(CThreadApp::threadProc, this));
    if ( nresult == ESUCCESS ) {
        log_info(L_GEN, "Send SIGTERM (or press Ctrl-C) to terminate...\n");
    }
    else {
        log_error(L_GEN, "initialisation failed, error %d\n", nresult);
        CApplication::terminate();
    }

    return nresult;
}

/*
 * Application termination
 */
void CThreadApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
    m_thWorker.stop();

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    CThreadApp  app(argc, argv);
    return app.run();
}
