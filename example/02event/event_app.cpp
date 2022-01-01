/*
 *	Carbon Framework Examples
 *	Event send/receive
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 01.06.2016 11:50:35
 *	    Initial revision.
 */

#include "receiver.h"
#include "event_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CEventApp::CEventApp(int argc, char* argv[]) :
    CApplication("Event Application", MAKE_VERSION(1,0,0), 1, argc, argv),
    m_pReceiver(0)
{
}

/*
 * Application class destructor
 */
CEventApp::~CEventApp()
{
    shell_assert(m_pReceiver == 0);
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CEventApp::init()
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
        CEvent*     pEvent;

        /*
         * Create an event and send it to the m_pReceiver module
         * within the same event loop
         */
        log_info(L_GEN, "Sending Test event...\n");

        pEvent = new CEvent(EV_TEST, m_pReceiver, (PPARAM)0x1122, (NPARAM)0xEE55, "Test event");
        appSendEvent(pEvent);

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
void CEventApp::terminate()
{
    CModule* arModule[] = { m_pReceiver };

    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
    appTerminateModules(arModule, ARRAY_SIZE(arModule));

    SAFE_DELETE(m_pReceiver);

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    CEventApp   app(argc, argv);
    return app.run();
}
