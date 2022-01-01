/*
 *	Carbon Framework Examples
 *	Network server client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 27.06.2016 11:32:45
 *	    Initial revision.
 */
/*
 * Example:
 *
 *      1) Connect to the remove server NET_INTERFACE:NET_PORT
 *      2) Awaiting for the connection notification
 *
 *      3) Send a VEP packet to the remote server
 *      4) Awaiting for the send notification
 *
 *      5) Awaiting for the reply VEP packet from the server
 *
 *      6) Send and Receive (Io()) a VEP packet
 *
 *      7) Close the connection and exit application
 */

#include <unistd.h>

#include "vep/vep.h"
#include "vep/packet_system.h"

#include "shared.h"
#include "net_client_app.h"

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CNetClientApp::CNetClientApp(int argc, char* argv[]) :
    CApplication("Net Client Application", MAKE_VERSION(1,0,0), 1, argc, argv),
    m_netClient(this),
    m_sessId(0),
    m_sessIdIo(0)
{
}

/*
 * Application class destructor
 */
CNetClientApp::~CNetClientApp()
{
    shell_assert(m_netClient.isConnected() != ESUCCESS);
}

/*
 * Disconnect from the server and close all sessions
 */
void CNetClientApp::disconnect()
{
    if ( m_netClient.isConnected() == ESUCCESS )  {
        m_netClient.disconnect();
        usleep(500000);
    }

    m_sessId = 0;
    m_sessIdIo = 0;
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
boolean_t CNetClientApp::processEvent(CEvent* pEvent)
{
    CEventNetClientRecv*        pEventRecv;
    dec_ptr<CVepContainer>      pContainer;
    result_t                    nresult;
    seqnum_t                    sessId;
    boolean_t                   bProcessed;

    switch ( pEvent->getType() )  {
        case EV_NET_CLIENT_CONNECTED:
            nresult = (result_t)pEvent->getnParam();

            if ( nresult == ESUCCESS )  {
                log_info(L_GEN, "*** Remote Server connected ***\n");

                log_info(L_GEN, "*** Sending a VEP packet ***\n");
                pContainer = new CVepContainer(VEP_CONTAINER_SYSTEM, 22);
                shell_assert(m_sessId == 0);
                m_sessId = getUniqueId();
                m_netClient.send(pContainer, m_sessId);
            }
            else {
                log_error(L_GEN, "*** Failed to connect to server, result %d ***\n", nresult);

				/* Quit allication */
				appSendEvent(new CEvent(EV_QUIT, this, NULL, 1));
            }

            bProcessed = TRUE;
            break;

        case EV_NET_CLIENT_SENT:
            sessId = pEvent->getSessId();

            if ( m_sessId == sessId ) {
                m_sessId = 0;

                nresult = (result_t)pEvent->getnParam();
                if ( nresult == ESUCCESS) {
                    dec_ptr<CVepContainer>  pRecvContainer = new CVepContainer;

                    log_info(L_GEN, "*** Packet has been sent ***\n");

                    log_info(L_GEN, "*** Awaiting a reply VEP Packet... ***\n");
                    m_sessId = getUniqueId();
                    m_netClient.recv(pRecvContainer, m_sessId);
                }
                else {
                    log_error(L_GEN, "*** Packet sent failure, result %d ***\n", nresult);
                }
            }

            bProcessed = TRUE;
            break;

        case EV_NET_CLIENT_RECV:
            pEventRecv = dynamic_cast<CEventNetClientRecv*>(pEvent);
            shell_assert(pEventRecv);

            sessId = pEventRecv->getSessId();
            if (  sessId == m_sessId || sessId == m_sessIdIo )  {

                nresult = pEventRecv->getResult();
                if ( nresult == ESUCCESS )  {
                    log_info(L_GEN, "*** A reply VEP packet has been received:\n");
                    pEventRecv->getContainer()->dump();
                }
                else {
                    log_error(L_GEN, "*** A reply packet receive failure, result %d ***\n", nresult);
                }

                if ( sessId == m_sessId )  {
                    dec_ptr<CVepContainer>      pContainer;

                    m_sessId = 0;

                    log_info(L_GEN, "*** Io (send/receive packet) ***\n");

                    pContainer = new CVepContainer(VEP_CONTAINER_SYSTEM, 2);
                    m_sessIdIo = getUniqueId();
                    m_netClient.io(pContainer, m_sessIdIo);
                }
                else {
                    /* Disconnect client and exit application */
                    disconnect();
                    CApplication::stopApplication(0);
                }
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
result_t CNetClientApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* User application inialisation */
    initSystemPacket();

    nresult = m_netClient.init();
    if ( nresult == ESUCCESS )  {
        CNetAddr    connectAddr(NET_INTERFACE, NET_PORT);

		if ( !connectAddr.isValid() )  {
			log_error(L_GEN, "interface %s is not found, connecting to interface 'lo'\n",
					  	NET_INTERFACE);
			connectAddr.setHost("lo");
		}

        m_netClient.connect(connectAddr, getUniqueId());
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
void CNetClientApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");


    /* User application termination */
    disconnect();
    m_netClient.terminate();

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    CNetClientApp  app(argc, argv);
    return app.run();
}
