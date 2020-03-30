/*
 *	Carbon Framework Examples
 *	Remote Event receive server
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 03.08.2016 17:23:05
 *	    Initial revision.
 */

#include "remote_event_recv_app.h"
#include "shared.h"

#if CARBON_DEBUG_EVENT
/*
 * Event names (for debugging purpose)
 */
static const char* eventStringTable[] =
{
    "EV_R_TEST",
    "EV_R_TEST_REPLY"
};
#endif /* CARBON_DEBUG_EVENT */

/*
 * Application class constructor
 *
 *      argc        command line argument 'argc'
 *      argv        command line argument 'argv'
 */
CRemoteEventRecvApp::CRemoteEventRecvApp(int argc, char* argv[]) :
    CApplication("Remote Event Receiver Application", MAKE_VERSION(1,0), 1, argc, argv),
	m_nEventCount(0)
{
}

/*
 * Application class destructor
 */
CRemoteEventRecvApp::~CRemoteEventRecvApp()
{
}

/*
 * Dump remote event
 *
 * 		pEvent		remote event to dump parameters
 */
void CRemoteEventRecvApp::dumpEvent(CRemoteEvent* pEvent) const
{
    CString         strPayload;
    const char*     strData = (const char*)pEvent->getData();
    size_t          size = pEvent->getDataSize();

    pEvent->dump();

    if ( strData != 0 && size > 0 )  {
        size = sh_min(size, 16536);
        strPayload.append(strData, size);
        log_dump("\t\t ool data: '%s'\n", (const char*)strPayload);
    }
}

/*
 * Send reply event for a received remote event
 *
 * 		pEvent		received remote event
 */
void CRemoteEventRecvApp::replyEvent(CRemoteEvent* pEvent)
{
    CRemoteEvent*   pReplyEvent;
    const char*		strData = "Reply payload data.";

    pReplyEvent = new CRemoteEvent(EV_R_TEST_REPLY, pEvent->getReplyReceiver(), 0,
                                   pEvent->getSessId(),
								   strData, strlen(strData)+1);
    appSendRemoteEvent(pReplyEvent, pEvent->getReplyRid());
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
boolean_t CRemoteEventRecvApp::processEvent(CEvent* pEvent)
{
    CRemoteEvent*   pRemoteEvent;
    boolean_t       bProcessed;

    switch ( pEvent->getType() )  {
        case EV_R_TEST:
            pRemoteEvent = dynamic_cast<CRemoteEvent*>(pEvent);
            shell_assert(pRemoteEvent != 0);
            if ( pRemoteEvent )  {
                log_info(L_GEN, "[app] remote event %d has been received\n",
                         pEvent->getType());
                dumpEvent(pRemoteEvent);
                replyEvent(pRemoteEvent);
            }
            else {
                log_error(L_GEN, "[app] event %d is not a remote event!\n",
                          pEvent->getType());
            }

			if ( ++m_nEventCount >= 3 ) {
				/* Receive 3 events and exit application */
				stopApplication(222);
			}

            bProcessed = TRUE;
            break;

        default:
            bProcessed = CApplication::processEvent(pEvent);
            break;
    }

    return bProcessed;
}

void CRemoteEventRecvApp::initEvent()
{
    CApplication::initEvent();

#if CARBON_DEBUG_EVENT
    /* Register additional event names (for debugging) */
    CEventStringTable::registerStringTable(EV_R_TEST, EV_R_TEST_REPLY, eventStringTable);
#endif /* CARBON_DEBUG_EVENT */
}

/*
 * Application initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CRemoteEventRecvApp::init()
{
    result_t    nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

    /* The Remote Event Service is not initialised automatically */
    nresult = initRemoteEventService(CARBON_REMOTE_EVENT_RECV_RID, this);
    if ( nresult != ESUCCESS )  {
        CApplication::terminate();
        return nresult;
    }

    return nresult;
}

/*
 * Application termination
 */
void CRemoteEventRecvApp::terminate()
{
    log_info(L_GEN, "Custom application termination...\n");

    /* User application termination */
    exitRemoteEventService();

    CApplication::terminate();
}

/*
 * Normal C/C++ entry point
 */
int main(int argc, char* argv[])
{
    return CRemoteEventRecvApp(argc, argv).run();
}
