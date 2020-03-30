/*
 *  Carbon Framework Network Center
 *  Main module
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 01.06.2015 14:17:56
 *      Initial revision.
 */

#include "carbon/carbon.h"
#include "vep/packet_system.h"

#include "event.h"
#include "packet_center.h"
#include "center.h"

#define CENTER_NAME             "Apollo Center"
#define CENTER_VERSION          MAKE_VERSION(0, 1)


/*******************************************************************************
 * Main module CCenter
 */

CCenterApp::CCenterApp(int argc, char* argv[]) :
    CApplication(CENTER_NAME, CENTER_VERSION, 1, argc, argv),
    m_pNetConnector(0),
    m_pSysResponder(0),
    m_pHeartBeat(0),
    m_pHostList(0)
{
}

CCenterApp::~CCenterApp()
{
    shell_assert(m_pNetConnector == 0);
    shell_assert(m_pSysResponder == 0);
    shell_assert(m_pHeartBeat == 0);
    shell_assert(m_pHostList == 0);
}

void CCenterApp::initLogger()
{
    CApplication::initLogger();
    //logger_enable(LT_DEBUG|L_NET_FL);
    //logger_enable(LT_ALL|L_EV_TRACE_EVENT);
    logger_enable(LT_ALL|L_NETCONN_IO);
}

/*
 * Initialisation
 *
 * Return:
 *      ESUCCESS        success
 *      ...             failed, exit application
 */
result_t CCenterApp::init()
{
    CNetAddr    listenAddr("127.0.0.1", 10001);
    CNetAddr    bindAddr("ens33", INADDR_ANY);
    result_t    nresult;

    if ( !bindAddr.isValid() )  {
        bindAddr.setHost("eth1");
    }

    shell_assert(listenAddr.isValid());
    shell_assert(bindAddr.isValid());

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    initCenterEvent();          /* Initialise event string table */
	initSystemPacket();
    initCenterPacket();         /* Initialise packet string table */

    m_pNetConnector = new CTcpConnector(new CVepContainer, this, 20);
    m_pNetConnector->setBindAddr(bindAddr);
    m_pSysResponder = new CSysResponder(this, m_pNetConnector);
    m_pHostList = new CHostList(this);
    m_pHeartBeat = new CHeartBeat(this, bindAddr);

    CModule* arModules[] = { m_pNetConnector, m_pSysResponder,
                             m_pHeartBeat, m_pHostList };

    nresult = appInitModules(arModules, ARRAY_SIZE(arModules));
    if ( nresult == ESUCCESS ) {
        nresult = m_pNetConnector->startListen(listenAddr);
        if ( nresult == ESUCCESS ) {
            m_pHostList->startHeartBeat();
        }
        else {
            appTerminateModules(arModules, ARRAY_SIZE(arModules));
        }
    }

    return nresult;
}


void CCenterApp::terminate()
{
    CModule* arModules[] = { m_pNetConnector, m_pSysResponder,
                             m_pHeartBeat, m_pHostList };
    m_pNetConnector->stopListen();
    appTerminateModules(arModules, ARRAY_SIZE(arModules));

    m_pNetConnector->dump();

    SAFE_DELETE(m_pNetConnector);
    SAFE_DELETE(m_pSysResponder);
    SAFE_DELETE(m_pHeartBeat);
    SAFE_DELETE(m_pHostList);

    CApplication::terminate();
}

void CCenterApp::processEventPacket(CEventTcpConnRecv* pEventRecv)
{
    CVepContainer*      pContainer = dynamic_cast<CVepContainer*>(pEventRecv->getContainer());

    shell_assert(pContainer->isValid());

    switch ( pContainer->getType() )  {
        case VEP_CONTAINER_SYSTEM:
            m_pSysResponder->processPacket(pEventRecv);
            break;

        case VEP_CONTAINER_CENTER:
            pEventRecv->reference();
            pEventRecv->setReceiver(m_pHostList);
            appSendEvent(pEventRecv);
            break;

        default:
            log_debug(L_GEN, "[center] dropped unsupported packet, dump following\n");
            pContainer->dump();
            break;
    }
}


boolean_t CCenterApp::processEvent(CEvent* pEvent)
{
    CEventTcpConnRecv*  pEventRecv;
    boolean_t           bProcessed = CApplication::processEvent(pEvent);

    switch ( pEvent->getType() )  {
        case EV_NETCONN_RECV:
            pEventRecv = dynamic_cast<CEventTcpConnRecv*>(pEvent);
            shell_assert(pEventRecv);
            if ( pEventRecv )  {
                processEventPacket(pEventRecv);
            }
            bProcessed = TRUE;
            break;
    }

    return bProcessed;
}


int main(int argc, char* argv[])
{
    CCenterApp  theApp(argc, argv);

    return theApp.run();
}
