/*
 *	Carbon Framework Examples
 *	Timer application
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 01.06.2016 15:34:10
 *	    Initial revision.
 */

#include "shell/hr_time.h"

#include "receiver.h"
#include "timer_app.h"

#define TIMER_INTERVAL          HR_2SEC

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CTimerApp::CTimerApp(int argc, char* argv[]) :
    CApplication("Timer Application", MAKE_VERSION(1,0,0), 1, argc, argv),
    m_pReceiver(0),
    m_pTimer(0)
{
}

/*
 * Application class destructor
 */
CTimerApp::~CTimerApp()
{
    shell_assert(m_pReceiver == 0);
}

/*
 * Create and start periodic timer with TIMER_INTERVAL
 */
void CTimerApp::startTimer()
{
    shell_assert(m_pTimer == 0);

    m_pTimer = new CTimer(TIMER_INTERVAL, TIMER_CALLBACK(CTimerApp::timerHandler, this),
                            CTimer::timerPeriodic, NULL, "test-timer");
    appMainLoop()->insertTimer(m_pTimer);
}

/*
 * Stop and detele timer
 */
void CTimerApp::stopTimer()
{
    SAFE_DELETE_TIMER(m_pTimer, appMainLoop());
}

/*
 * Procedure called by timer
 *
 *      p       transparent data pointer passed to timer constructor
 */
void CTimerApp::timerHandler(void* p)
{
    CEvent*     pEvent;
    static int  n = 0;

    log_info(L_GEN, "Sending Test event...\n");

    pEvent = new CEvent(EV_TEST, m_pReceiver, (PPARAM)0x1122, (NPARAM)n++, "Test event");
    appSendEvent(pEvent);
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CTimerApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* User application inialisation */
    m_pReceiver = new CReceiverModule(this);

    CModule* arModule[] = { m_pReceiver };

    nresult = appInitModules(arModule, ARRAY_SIZE(arModule));
    if ( nresult == ESUCCESS ) {
        startTimer();

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
void CTimerApp::terminate()
{
    CModule* arModule[] = { m_pReceiver };

    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
    stopTimer();

    appTerminateModules(arModule, ARRAY_SIZE(arModule));

    SAFE_DELETE(m_pReceiver);

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
#if __unix__
int main(int argc, char* argv[])
#else /* __unix__ */
int __embed_main(int argc, char* argv[])
#endif /* __unix__ */
{
    CTimerApp   app(argc, argv);
    return app.run();
}
