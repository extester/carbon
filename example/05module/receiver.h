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
 *	Revision 1.0, 26.01.2017 12:05:46
 *	    Initial revision.
 */

#ifndef __EXAMPLE_RECEIVER_H_INCLUDED__
#define __EXAMPLE_RECEIVER_H_INCLUDED__

#include "shell/counter.h"

#include "carbon/carbon.h"
#include "carbon/module.h"
#include "carbon/event.h"

#define EV_KEY			(EV_USER+0)

class CModuleApp;

class CReceiverModule : public CModule, public CEventReceiver
{
    protected:
        CModuleApp*      m_pParent;		/* Parent application pointer */

		counter_t		m_nKeys;		/* Stat: keypress processed counter */

    public:
        CReceiverModule(CModuleApp* pParent);
        virtual ~CReceiverModule();

	public:
        virtual result_t init();
        virtual void terminate();

		virtual void dump(const char* strPref = "") const;

    protected:
        virtual boolean_t processEvent(CEvent* pEvent);
};


#endif /* __EXAMPLE_RECEIVER_H_INCLUDED__ */
