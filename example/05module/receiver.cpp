/*
 *	Carbon Framework Examples
 *	Event receiver module
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 26.01.2017 12:06:19
 *	    Initial revision.
 */

#include "module_app.h"
#include "receiver.h"

#define MODULE_NAME			"Receiver"

/*
 * Module class constructor
 *
 *      pParent     parent application pointer
 */
CReceiverModule::CReceiverModule(CModuleApp* pParent) :
    CModule(MODULE_NAME),
    CEventReceiver(pParent, MODULE_NAME),
    m_pParent(pParent),
    m_nKeys(ZERO_COUNTER)
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
    int         key;
    boolean_t   bProcessed = FALSE;

    switch ( pEvent->getType() )  {
        case EV_KEY:
            key = (int)pEvent->getnParam();

            log_info(L_GEN, "*** Key pressed, code %d (%02Xh) ***\n\r", key, key);
            counter_inc(m_nKeys);

            if ( key == 27 )  {
                m_pParent->sendEvent(new CEvent(EV_QUIT, m_pParent, NULL, 0));
            }

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

    log_info(L_GEN, "Custom Receiver module initialisation...\n\r");

    /* User module inialisation */

    return ESUCCESS;
}

/*
 * Module termination
 */
void CReceiverModule::terminate()
{
    log_info(L_GEN, "Custom Receiver module termination...\n\r");

    /* User module termination */
    dump("Receiver statistic: ");

    CModule::terminate();
}

/*
 * Debugging support
 */
void CReceiverModule::dump(const char* strPref) const
{
    log_dump("%s%d key(s) pressed\n\r", strPref, counter_get(m_nKeys));
}
