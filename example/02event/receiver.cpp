/*
 *	Carbon Framework Examples
 *	Event send/receive
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 01.06.2016 12:23:58
 *	    Initial revision.
 */

#include "event_app.h"
#include "receiver.h"

/*
 * Module class constructor
 *
 *      pParent     parent application pointer
 */
CReceiverModule::CReceiverModule(CEventApp* pParent) :
    CModule("Receiver"),
    CEventReceiver(appMainLoop(), "Receiver"),
    m_pParent(pParent),
    m_nEvents(ZERO_COUNTER)
{
}

/*
 * Module class destructor
 */
CReceiverModule::~CReceiverModule()
{
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
boolean_t CReceiverModule::processEvent(CEvent* pEvent)
{
    boolean_t   bProcessed = FALSE;

    switch ( pEvent->getType() )  {
        case EV_TEST:
            log_info(L_GEN, "*** Received Test event [0x%X, 0x%X] ***\n",
                pEvent->getpParam(), pEvent->getnParam());
            counter_inc(m_nEvents);
            bProcessed = TRUE;
            break;
    }

    return bProcessed;
}

/*
 * Module initialisation
 *
 * Return:
 *      ESUCCESS        initalisation success
 *      other code      initalisation failed, exit application
 */
result_t CReceiverModule::init()
{
    result_t    nresult;

    nresult = CModule::init();
    if ( nresult != ESUCCESS )  {
        return nresult;
    }

    log_info(L_GEN, "Custom Receiver module initialisation...\n");

    /* User module inialisation */

    return ESUCCESS;
}

/*
 * Module termination
 */
void CReceiverModule::terminate()
{
    log_info(L_GEN, "Custom Receiver module termination...\n");

    /* User module termination */
    dump("Receiver statistic: ");

    CModule::terminate();
}

/*
 * Debugging support
 */
void CReceiverModule::dump(const char* strPref) const
{
    log_dump("%s%d event(s) received\n", strPref, counter_get(m_nEvents));
}
