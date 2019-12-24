/*
 *	Carbon Framework Examples
 *	UDP network I/O server
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 08.09.2016 11:23:22
 *	    Initial revision.
 */

#include "carbon/raw_container.h"

#include "shared.h"
#include "udp_server_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CUdpServerApp::CUdpServerApp(int argc, char* argv[]) :
    CApplication("UDP Net Server Application", MAKE_VERSION(1,0), 1, argc, argv),
    m_netConnector(new CRawContainer(1024), this)
{
}

/*
 * Application class destructor
 */
CUdpServerApp::~CUdpServerApp()
{
}

result_t CUdpServerApp::startServer()
{
	CNetAddr	listenAddr(INADDR_ANY, NET_PORT);
	result_t	nresult;

	if ( !listenAddr.isValid() )  {
		log_error(L_GEN, "invalid listen address\n");
		return EINVAL;
	}

	nresult = m_netConnector.startListen(listenAddr);
	return nresult;
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
boolean_t CUdpServerApp::processEvent(CEvent* pEvent)
{
 	CEventUdpRecv*		pEventRecv;
    boolean_t           bProcessed;

    switch ( pEvent->getType() ) {
		case EV_NETCONN_RECV:
			pEventRecv = dynamic_cast<CEventUdpRecv*>(pEvent);
			shell_assert(pEventRecv);

			log_info(L_GEN, "*** Received a message from %s ***\n",
					 (const char*)pEventRecv->getSrcAddr());
			pEventRecv->getContainer()->dump();

            bProcessed = TRUE;
            break;

		default:
			bProcessed = CApplication::processEvent(pEvent);
			break;
   	}

    return bProcessed;
}

void CUdpServerApp::initLogger()
{
	CApplication::initLogger();

	//logger_enable(LT_ALL|L_NETCONN_IO);
	//logger_enable(LT_ALL|L_NETCONN_FL);
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CUdpServerApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* User application inialisation */
    nresult = m_netConnector.init();
    if ( nresult == ESUCCESS )  {
		nresult = startServer();
		if ( nresult == ESUCCESS ) {
			log_info(L_GEN, "Send SIGTERM (or press Ctrl-C) to terminate...\n");
		}
		else {
			m_netConnector.terminate();
			CApplication::terminate();
		}
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
void CUdpServerApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
    m_netConnector.terminate();

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
	return CUdpServerApp(argc, argv).run();
}
