/*
 *	Carbon Framework Examples
 *	Main application
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 26.01.2017 12:04:08
 *	    Initial revision.
 */

#include "receiver.h"
#include "keyboard.h"
#include "module_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CModuleApp::CModuleApp(int argc, char* argv[]) :
    CApplication("Module Application", MAKE_VERSION(1,0), 1, argc, argv),
    m_pReceiver(0),
    m_pKeyboard(0)
{
}

/*
 * Application class destructor
 */
CModuleApp::~CModuleApp()
{
    shell_assert(m_pReceiver == 0);
    shell_assert(m_pKeyboard == 0);
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CModuleApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n\r");

    /* User application inialisation */
    m_pReceiver = new CReceiverModule(this);
    m_pKeyboard = new CKeyboardModule(this, m_pReceiver);

    CModule* arModule[] = { m_pKeyboard, m_pReceiver };

    nresult = appInitModules(arModule, ARRAY_SIZE(arModule));
    if ( nresult == ESUCCESS ) {
        log_info(L_GEN, "Press ESC to terminate...\n\r");
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
void CModuleApp::terminate()
{
    CModule* arModule[] = { m_pKeyboard, m_pReceiver };

    log_info(L_GEN, "Custom application termination...\n\r");

    /* User application termination */
    appTerminateModules(arModule, ARRAY_SIZE(arModule));

    SAFE_DELETE(m_pKeyboard);
    SAFE_DELETE(m_pReceiver);

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    CModuleApp   app(argc, argv);
    return app.run();
}
