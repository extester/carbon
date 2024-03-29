/*
 *	Carbon Framework Examples
 *	Network server synchronous I/O
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 05.09.2016 17:28:43
 *	    Initial revision.
 */

#include "vep/vep.h"
#include "vep/packet_system.h"

#include "shared.h"
#include "net_server_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CNetServerApp::CNetServerApp(int argc, char* argv[]) :
    CApplication("Net Server Synchronous Application", MAKE_VERSION(1,0,0), 1, argc, argv),
    m_netServer(1/*single client*/, new CVepContainer, this),
    m_hConnection(0)
{
}

/*
 * Application class destructor
 */
CNetServerApp::~CNetServerApp()
{
}

void CNetServerApp::initLogger()
{
	CApplication::initLogger();
}

/*
 * Disconnect from the client and close all sessions
 */
void CNetServerApp::disconnect()
{
    if ( m_hConnection )  {
        m_netServer.disconnect(m_hConnection);
        m_hConnection = 0;
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
boolean_t CNetServerApp::processEvent(CEvent* pEvent)
{
    CEventNetServerRecv*        	pEventRecv;
    CVepContainer*              	pContainer;
    dec_ptr<CVepContainer>      	pReplyContainer;
	system_packet_version_reply_t   versionReply;
    result_t                    	nresult;
    boolean_t                   	bProcessed;

    switch ( pEvent->getType() )  {
        case EV_NET_SERVER_CONNECTED:
			/* Allow single client only */
            shell_assert(m_hConnection == 0);

            log_info(L_GEN, "*** Connected Client ***\n");
            m_hConnection = (net_connection_t)pEvent->getpParam();

            bProcessed = TRUE;
            break;

		case EV_NET_SERVER_DISCONNECTED:
			shell_assert(m_hConnection != 0);

			if ( (net_connection_t)pEvent->getpParam() == m_hConnection )  {
				log_info(L_GEN, "*** Connection Closed by Peer ***\n");
				disconnect();
			}

			bProcessed = TRUE;
			break;

        case EV_NET_SERVER_RECV:
            pEventRecv = dynamic_cast<CEventNetServerRecv*>(pEvent);
            shell_assert(pEventRecv);

			log_info(L_GEN, "*** Received a VEP packet:\n");

            pContainer = dynamic_cast<CVepContainer*>(pEventRecv->getContainer());
            shell_assert(pContainer);
            pContainer->dump();

            log_info(L_GEN, "*** Sending a reply VEP packet ***\n");

			pReplyContainer = new CVepContainer(VEP_CONTAINER_SYSTEM);
			versionReply.lib_version = carbon_get_version();
			versionReply.version = getVersion();

			nresult = pReplyContainer->insertPacket(SYSTEM_PACKET_VERSION_REPLY,
													&versionReply, sizeof(versionReply));
			shell_assert(nresult == ESUCCESS);

            nresult = m_netServer.sendSync(pReplyContainer, m_hConnection);
			if ( nresult != ESUCCESS )  {
				log_error(L_GEN, "failed to send reply container, result: %d\n", nresult);
			}

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
result_t CNetServerApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* User application inialisation */
	initSystemPacket();

    nresult = m_netServer.init();
    if ( nresult == ESUCCESS )  {
        CNetAddr    listenAddr(NET_INTERFACE, NET_PORT);

		if ( !listenAddr.isValid() )  {
			log_error(L_GEN, "interface %s is not found, listenning on all interfaces\n",
					  NET_INTERFACE);
			listenAddr.setHost("0.0.0.0");
		}

        m_netServer.startListen(listenAddr);
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
void CNetServerApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
    disconnect();
    m_netServer.terminate();

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    return CNetServerApp(argc, argv).run();
}
