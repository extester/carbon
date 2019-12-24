/*
 *	Carbon Framework Examples
 *	Linux driver communication
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 27.01.2017 17:51:22
 *	    Initial revision.
 */

#include "driver.h"
#include "driver_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CDriverApp::CDriverApp(int argc, char* argv[]) :
    CApplication("Driver Application", MAKE_VERSION(1,0), 1, argc, argv),
	m_pDriver(0)
{
}

/*
 * Application class destructor
 */
CDriverApp::~CDriverApp()
{
    shell_assert(m_pDriver == 0);
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
boolean_t CDriverApp::processEvent(CEvent* pEvent)
{
	uint32_t	data;
	boolean_t   bProcessed;

	switch( pEvent->getType() )  {
		case EV_DRIVER_MESSAGE:
			data  = (uint32_t)pEvent->getnParam();

			log_info(L_GEN, "*** Driver message, data: %u ***\n", data);
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
result_t CDriverApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n\r");

    /* User application inialisation */
	m_pDriver = new CDriverModule(this);

	nresult = m_pDriver->init();
    if ( nresult == ESUCCESS ) {
		log_info(L_GEN, "Send SIGTERM (or press Ctrl-C) to terminate...\n");
    }
    else {
        log_error(L_GEN, "initialisation failed, error %d\n", nresult);
		SAFE_DELETE(m_pDriver);
        CApplication::terminate();
    }

    return nresult;
}

/*
 * Application termination
 */
void CDriverApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n\r");

    /* User application termination */
	m_pDriver->terminate();

    SAFE_DELETE(m_pDriver);

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    CDriverApp   app(argc, argv);
    return app.run();
}
