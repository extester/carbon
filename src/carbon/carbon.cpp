/*
 *  Carbon framework
 *  Library common definitions, types and includes
 *
 *  Copyright (c) 2015-2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 29.05.2015 18:12:51
 *      Initial revision.
 *
 *  Revision 2.0, 15.02.2017 17:16:48
 *  	Separated to common and machine-dependent parts
 */

#include "carbon/event.h"
#include "carbon/event/eventloop.h"
#include "carbon/application.h"
#include "carbon/version.h"
#include "carbon/carbon.h"

/*
 * Get Carbon library version
 *
 *  Return: version
 */
version_t carbon_get_version()
{
    return CARBON_LIBRARY_VERSION;
}

/*
 * Get unique 32-bits idendificator
 *
 * Return: identificator
 *
 * Note: thread-safe, monotonic increasing
 */
seqnum_t getUniqueId()
{
    static atomic_t	uniqueId = ZERO_ATOMIC;
    seqnum_t		id;

    do {
        id = (seqnum_t)atomic_inc(&uniqueId);
    } while ( id == NO_SEQNUM );

    return id;
}

/*
 * Sending event to the event-loop/receiver
 *
 *      pEvent      event to send
 *
 * Note: function used a single reference of the object
 */
void appSendEvent(CEvent* pEvent)
{
    CEventReceiver*     pReceiver;
    CEventLoop*			pLoop;

    shell_assert(pEvent);
    pReceiver = pEvent->getReceiver();

    shell_assert_ex(pReceiver != EVENT_MULTICAST, "appSendEvent() doesn't support "
        "multicast receivers, use appSendMulticastEvent()!\n");

    if ( pReceiver != EVENT_MULTICAST )  {
        pLoop = pReceiver->getEventLoop();
        if ( pLoop )  {
            pLoop->sendEvent(pEvent);
        }
        else {
            log_debug(L_GEN, "[app] no such Event Receiver (%Xh) found!\n", A(pReceiver));
            pEvent->release();
        }
    }
    else {
        pEvent->release();
    }
}

/*
 * Sending event to the all receivers of the specified event loop
 *
 *      pEvent      event to send
 *      pLoop       eventloop to process
 *
 * Note: the event's receiver must be set to EVENT_MULTICAST
 */
void appSendMulticastEvent(CEvent* pEvent, CEventLoop* pLoop)
{
    shell_assert(pEvent);
    shell_assert(pEvent->getReceiver() == EVENT_MULTICAST);
    shell_assert(pLoop);
    pLoop->sendEvent(pEvent);
}

/*
 * Get main eventloop of the current application
 *
 * Return: eventloop pointer
 */
CEventLoop* appMainLoop()
{
    shell_assert(appApplication);
    shell_assert(appApplication->getMainLoop());
    return appApplication->getMainLoop();
}
