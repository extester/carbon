/*
 *	Carbon Framework Examples
 *	Minimal program
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 01.06.2016 10:51:22
 *	    Initial revision.
 */

#include "minimal_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CMinimalApp::CMinimalApp(int argc, char* argv[]) :
    CApplication("Minimal Application", MAKE_VERSION(1,0), 1, argc, argv)
{
}

/*
 * Application class destructor
 */
CMinimalApp::~CMinimalApp()
{
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CMinimalApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* === Start user custom initialisation === */

    /* === End user custom initialisation === */

    log_info(L_GEN, "Send SIGTERM (or press Ctrl-C) to terminate...\n");

    return ESUCCESS;
}

/*
 * Application termination
 */
void CMinimalApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");

    /* === Start user custom termination === */

    /* === End user custom termination === */

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    CMinimalApp app(argc, argv);
    return app.run();
}
