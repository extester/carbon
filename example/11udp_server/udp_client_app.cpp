/*
 *	Carbon Framework Examples
 *	UDP network I/O client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 07.09.2016 18:37:09
 *	    Initial revision.
 */

#include "carbon/raw_container.h"

#include "shared.h"
#include "udp_client_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CUdpClientApp::CUdpClientApp(int argc, char* argv[]) :
    CApplication("UDP Net Client Application", MAKE_VERSION(1,0), 1, argc, argv),
    m_netConnector(new CRawContainer(1024), this),
	m_pTimer(0)
{
}

/*
 * Application class destructor
 */
CUdpClientApp::~CUdpClientApp()
{
	shell_assert(m_pTimer == 0);
}

void CUdpClientApp::initLogger()
{
	CApplication::initLogger();

	//logger_enable(LT_ALL|L_NETCONN_IO);
	//logger_enable(LT_ALL|L_NETCONN_FL);
}

void CUdpClientApp::sendMessage()
{
	dec_ptr<CRawContainer>  pContainer = new CRawContainer(128);
	CNetAddr                dstAddr(INADDR_BROADCAST, NET_PORT);
	result_t                nresult;
	static int 				n = 0;
	static char 			buf[8];

	if ( dstAddr.isValid() )  {
		log_info(L_GEN, "*** sending a broadcast container to %s ***\n", (const char*)dstAddr);

		memset(buf, n++, sizeof(buf));
		nresult = pContainer->putData(buf, sizeof(buf));
		if ( nresult == ESUCCESS )  {
			nresult = m_netConnector.sendSync(pContainer, dstAddr);
			if ( nresult != ESUCCESS ) {
				log_error(L_GEN, "failed to send broadcast container, result %d\n", nresult);
			}
		}
	}
	else {
		log_error(L_GEN, "invalid address %s\n", (const char*)dstAddr);
	}
}

void CUdpClientApp::timerHandler(void* pData)
{
	shell_unused(pData);
	sendMessage();
}

void CUdpClientApp::stopTimer()
{
	SAFE_DELETE_TIMER(m_pTimer, appMainLoop());
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CUdpClientApp::init()
{
	CNetAddr	bindAddr("lo", INPORT_ANY);
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* User application inialisation */
	m_netConnector.setBindAddr(bindAddr);
    nresult = m_netConnector.init();
    if ( nresult == ESUCCESS )  {
		m_pTimer = new CTimer(HR_2SEC, TIMER_CALLBACK(CUdpClientApp::timerHandler, this),
							  CTimer::timerPeriodic, "send-timer");
		appMainLoop()->insertTimer(m_pTimer);

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
void CUdpClientApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
	stopTimer();
    m_netConnector.terminate();

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
	return CUdpClientApp(argc, argv).run();
}
