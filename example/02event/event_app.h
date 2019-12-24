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
 *	Revision 1.0, 01.06.2016 11:48:30
 *	    Initial revision.
 */

#ifndef __EXAMPLE_EVENT_H_INCLUDED__
#define __EXAMPLE_EVENT_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"

#include "receiver.h"
#include "event_app.h"

/*
 * User defined custom event
 */
#define EV_TEST                 (EV_USER+0)

class CEventApp : public CApplication
{
    protected:
        CReceiverModule*    m_pReceiver;		/* Receiver module */

    public:
        CEventApp(int argc, char* argv[]);
        virtual ~CEventApp();

	public:
        virtual result_t init();
        virtual void terminate();
};


#endif /* __EXAMPLE_EVENT_H_INCLUDED__ */
