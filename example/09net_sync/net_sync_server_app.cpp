/*
 *	Carbon Framework Examples
 *	Synchronous Network server
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 31.08.2016 17:15:17
 *	    Initial revision.
 */

#include "vep/vep.h"
#include "vep/packet_system.h"

#include "shared.h"
#include "net_sync_server_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CNetSyncServerApp::CNetSyncServerApp(int argc, char* argv[]) :
    CApplication("Net Sync Server Application", MAKE_VERSION(1,0,0), 1, argc, argv),
    m_netConnector(new CVepContainer, this)
{
}

/*
 * Application class destructor
 */
CNetSyncServerApp::~CNetSyncServerApp()
{
}

void CNetSyncServerApp::sendReply(CEventTcpConnRecv* pEvent)
{
	CSocketRef*             		pSocket;
	CVepContainer*          		pContainer;
	CVepPacket*             		pPacket;

	dec_ptr<CVepContainer>          pReplyContainer;
	system_packet_version_reply_t   versionReply;
	result_t                        nresult;


	shell_assert(pEvent != 0);
	shell_assert(pEvent->getResult() == ESUCCESS);

	pSocket = pEvent->getSocket();
	pContainer = dynamic_cast<CVepContainer*>(pEvent->getContainer());

	if ( !pSocket || !pContainer )  {
		log_error(L_GEN, "wrong packet event\n");
		return;
	}

	if ( !pContainer->isValid() || pContainer->getType() != VEP_CONTAINER_SYSTEM )  {
		log_error(L_GEN, "invalid or unexpected container type\n");
		return;
	}

	pPacket = (*pContainer)[0];
	switch ( pPacket->getType() )  {
		case SYSTEM_PACKET_VERSION:
			pReplyContainer = new CVepContainer(VEP_CONTAINER_SYSTEM);
			versionReply.lib_version = carbon_get_version();
			versionReply.version = getVersion();

			nresult = pReplyContainer->insertPacket(SYSTEM_PACKET_VERSION_REPLY,
													&versionReply, sizeof(versionReply));
			shell_assert(nresult == ESUCCESS);

			nresult = m_netConnector.sendSync(pReplyContainer, pSocket);
			if ( nresult != ESUCCESS )  {
				log_error(L_GEN, "failed to send reply container, result %d\n", nresult);
			}

			break;

		default:
			log_error(L_GEN, "received an unexpected VEP packet type %d\n", pPacket->getType());
			break;
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
boolean_t CNetSyncServerApp::processEvent(CEvent* pEvent)
{
    CVepContainer*          pContainer;
	CEventTcpConnRecv*		pEventRecv;
    boolean_t               bProcessed;

    switch ( pEvent->getType() )  {
        case EV_NETCONN_RECV:
			pEventRecv = dynamic_cast<CEventTcpConnRecv*>(pEvent);
			shell_assert(pEventRecv);
			shell_assert(pEventRecv->getResult() == ESUCCESS);

			log_info(L_GEN, "*** Received a VEP container: ***\n");

			pContainer = dynamic_cast<CVepContainer*>(pEventRecv->getContainer());
			pContainer->dump();

			log_info(L_GEN, "*** Sending a reply in synchronous mode ***\n");
			sendReply(pEventRecv);

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
result_t CNetSyncServerApp::init()
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
        CNetAddr    listenAddr(NET_INTERFACE, NET_PORT);

		if ( !listenAddr.isValid() )  {
			log_error(L_GEN, "interface %s is not found, listenning on all interfaces\n",
					  NET_INTERFACE);
			listenAddr.setHost("0.0.0.0");
		}

        nresult = m_netConnector.startListen(listenAddr);
		if ( nresult == ESUCCESS ) {
			log_info(L_GEN, "Send SIGTERM (or press Ctrl-C) to terminate...\n");
		}
		else {
			log_error(L_GEN, "listening failed, error %d\n", nresult);
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
void CNetSyncServerApp::terminate()
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
    return CNetSyncServerApp(argc, argv).run();
}
