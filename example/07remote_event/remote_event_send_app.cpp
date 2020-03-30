/*
 *	Carbon Framework Examples
 *	Remote Event sending client
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 03.08.2016 18:08:56
 *	    Initial revision.
 */

#include "remote_event_send_app.h"
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
CRemoteEventSendApp::CRemoteEventSendApp(int argc, char* argv[]) :
    CApplication("Remote Event Sender Application", MAKE_VERSION(1,0), 1, argc, argv),
	m_sessId(0)
{
}

/*
 * Application class destructor
 */
CRemoteEventSendApp::~CRemoteEventSendApp()
{
}

/*
 * Dump remote event
 *
 * 		pEvent		remote event to dump parameters
 */
void CRemoteEventSendApp::dumpEvent(CRemoteEvent* pEvent) const
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
 * Event processor
 *
 *      pEvent      event object to process
 *
 * Return:
 *      TRUE        event processed
 *      FALSE       event is not processed
 */
boolean_t CRemoteEventSendApp::processEvent(CEvent* pEvent)
{
    CRemoteEvent*   pRemoteEvent;
    boolean_t       bProcessed;

    switch ( pEvent->getType() )  {
        case EV_R_TEST_REPLY:
            pRemoteEvent = dynamic_cast<CRemoteEvent*>(pEvent);
            shell_assert(pRemoteEvent);
            if ( pRemoteEvent )  {
				if ( m_sessId == pRemoteEvent->getSessId() ) {
					log_info(L_GEN, "[app] remote event %d has been received\n",
							 pEvent->getType());
					dumpEvent(pRemoteEvent);

					/* Terminate application */
					stopApplication(333);
				}
            }
            else {
                log_error(L_GEN, "[app] event %d is not a remote event!\n",
                          pEvent->getType());
            }

            bProcessed = TRUE;
            break;

		default:
			bProcessed = CApplication::processEvent(pEvent);
			break;
    }

    return bProcessed;
}

void CRemoteEventSendApp::initEvent()
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
result_t CRemoteEventSendApp::init()
{
	result_t	nresult;

    nresult = CApplication::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom application initialisation...\n");

	/* Register additional event names (for debugging) */
#if CARBON_DEBUG_EVENT
	CEventStringTable::registerStringTable(EV_R_TEST, EV_R_TEST_REPLY, eventStringTable);
#endif /* CARBON_DEBUG_EVENT */

    /* The Remote Event Service is not initialised automatically */
    nresult = initRemoteEventService(CARBON_REMOTE_EVENT_SEND_RID, this);
    if ( nresult == ESUCCESS ) {
		CRemoteEvent*	pEvent;
		const char*		strData = "Example payload data.";

		m_sessId = getUniqueId();
		pEvent = new CRemoteEvent(EV_R_TEST, 0, this, m_sessId, strData, strlen(strData)+1);
		appSendRemoteEvent(pEvent, CARBON_REMOTE_EVENT_RECV_RID);
	}
	else {
        CApplication::terminate();
        return nresult;
    }

    return nresult;
}

/*
 * Application termination
 */
void CRemoteEventSendApp::terminate()
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
	return CRemoteEventSendApp(argc, argv).run();
}
