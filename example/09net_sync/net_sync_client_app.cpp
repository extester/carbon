/*
 *	Carbon Framework Examples
 *	Synchronous Network server client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 31.08.2016 17:14:33
 *	    Initial revision.
 */

#include "vep/vep.h"
#include "vep/packet_system.h"

#include "shared.h"
#include "net_sync_client_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CNetSyncClientApp::CNetSyncClientApp(int argc, char* argv[]) :
    CApplication("Net Sync Client Application", MAKE_VERSION(1,0), 1, argc, argv),
    m_netConnector(new CVepContainer, this),
	m_pTimer(0)
{
}

/*
 * Application class destructor
 */
CNetSyncClientApp::~CNetSyncClientApp()
{
	shell_assert(m_pTimer == 0);
}

void CNetSyncClientApp::timerHandler(void* pData)
{
	CNetAddr				servAddr(NET_INTERFACE, NET_PORT);
	result_t				nresult;
	dec_ptr<CVepContainer>	pContainer =
			new CVepContainer(VEP_CONTAINER_SYSTEM, SYSTEM_PACKET_VERSION);
	dec_ptr<CVepContainer>	pReplyContainer;

	SAFE_DELETE_TIMER(m_pTimer, appMainLoop());

	if ( servAddr.isValid() )  {
		log_info(L_GEN, "*** Requesting the system/library versions in sync mode ***\n");

		nresult = m_netConnector.ioSync(pContainer, servAddr, (CNetContainer**)&pReplyContainer);
		if ( nresult == ESUCCESS )  {
			CVepPacket*		pPacket;
			union systemPacketArgs*	args;

			if ( pReplyContainer->isValid() &&
					(pPacket=(*pReplyContainer)[0])->getType() == SYSTEM_PACKET_VERSION_REPLY )
			{
				args = (union systemPacketArgs*)pPacket->getHead();

				log_info(L_GEN, "*** Got the versions: library version %d.%d, system version %d.%d ***\n",
						 	VERSION_MAJOR(args->version_reply.lib_version),
						    VERSION_MINOR(args->version_reply.lib_version),
						    VERSION_MAJOR(args->version_reply.version),
						    VERSION_MINOR(args->version_reply.version));
			}
			else {
				log_error(L_GEN, "*** invalid or unexpected reply received ***\n");
			}
		}
		else {
			log_error(L_GEN, "*** request failed, result: %s(%d) ***\n",
					  strerror(nresult), nresult);
		}
	}
	else {
		log_error(L_GEN, "*** network interface %s is not found ***\n", NET_INTERFACE);
	}

	/* Exit application */
	CApplication::stopApplication(0);
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CNetSyncClientApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* User application inialisation */
	initSystemPacket();

    nresult = m_netConnector.init();
    if ( nresult == ESUCCESS )  {
		m_pTimer = new CTimer(HR_10MSEC, TIMER_CALLBACK(CNetSyncClientApp::timerHandler, this),
							  0, NULL, "bootstrap-timer");
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
void CNetSyncClientApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
	SAFE_DELETE_TIMER(m_pTimer, appMainLoop());
    m_netConnector.terminate();

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    return CNetSyncClientApp(argc, argv).run();
}
