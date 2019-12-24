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
 *	Revision 1.0, 01.06.2016 12:05:44
 *	    Initial revision.
 */

#ifndef __EXAMPLE_RECEIVER_H_INCLUDED__
#define __EXAMPLE_RECEIVER_H_INCLUDED__

#include "shell/counter.h"

#include "carbon/carbon.h"
#include "carbon/module.h"
#include "carbon/event.h"

class CEventApp;

class CReceiverModule : public CModule, public CEventReceiver
{
    protected:
        CEventApp*      m_pParent;		/* Parent application pointer */

		counter_t		m_nEvents;		/* Stat: event processed count */

    public:
        CReceiverModule(CEventApp* pParent);
        virtual ~CReceiverModule();

	public:
        virtual result_t init();
        virtual void terminate();

		virtual void dump(const char* strPref = "") const;

    protected:
        virtual boolean_t processEvent(CEvent* pEvent);
};


#endif /* __EXAMPLE_RECEIVER_H_INCLUDED__ */
