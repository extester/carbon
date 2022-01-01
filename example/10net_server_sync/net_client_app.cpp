/*
 *	Carbon Framework Examples
 *	Network server synchronous I/O client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 05.09.2016 18:11:55
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
    CApplication("Net Client Synchronous Application", MAKE_VERSION(1,0,0), 1, argc, argv),
    m_netClient(this),
    m_pTimer(0)
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
    if ( m_netClient.isConnected() == ESUCCESS ) {
        m_netClient.disconnect();
        sleep_ms(500);
    }
}

void CNetClientApp::timerHandler(void* pData)
{
    CNetAddr    connectAddr(NET_INTERFACE, NET_PORT);
    result_t    nresult;
    dec_ptr<CVepContainer>	pContainer =
                                new CVepContainer(VEP_CONTAINER_SYSTEM, SYSTEM_PACKET_VERSION);

    SAFE_DELETE_TIMER(m_pTimer, appMainLoop());

    if ( !connectAddr.isValid() )  {
        log_error(L_GEN, "interface %s is not found, connecting to interface 'lo'\n",
                  NET_INTERFACE);
        connectAddr.setHost("lo");
    }

    nresult = m_netClient.connectSync(connectAddr);
    if ( nresult == ESUCCESS )  {
        nresult = m_netClient.ioSync(pContainer);
        if ( nresult == ESUCCESS )  {
            CVepPacket*		        pPacket;
            union systemPacketArgs*	args;

            if ( pContainer->isValid() &&
                 (pPacket=(*pContainer)[0])->getType() == SYSTEM_PACKET_VERSION_REPLY )
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
            log_error(L_GEN, "*** request failed, result: %d ***\n", nresult);
        }
    }
    else {
        log_error(L_GEN, "*** failed to connect to %s, result: %d\n",
                  (const char*)connectAddr, nresult);
    }

	stopApplication(0);
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
        m_pTimer = new CTimer(HR_10MSEC, TIMER_CALLBACK(CNetClientApp::timerHandler, this),
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
    return CNetClientApp(argc, argv).run();
}
