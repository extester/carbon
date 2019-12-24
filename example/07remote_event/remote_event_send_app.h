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
 *	Revision 1.0, 03.08.2016 18:07:23
 *	    Initial revision.
 */

#ifndef __EXAMPLE_REMOTE_EVENT_SEND_APP_H_INCLUDED__
#define __EXAMPLE_REMOTE_EVENT_SEND_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/event/remote_event_service.h"

class CRemoteEventSendApp : public CApplication
{
	private:
		seqnum_t		m_sessId;		/* Unique session Id */

    public:
		CRemoteEventSendApp(int argc, char* argv[]);
        virtual ~CRemoteEventSendApp();

	public:
        virtual result_t init();
        virtual void terminate();

    protected:
        virtual boolean_t processEvent(CEvent* pEvent);
		virtual void initEvent();

	protected:
		void dumpEvent(CRemoteEvent* pEvent) const;

};


#endif /* __EXAMPLE_REMOTE_EVENT_SEND_APP_H_INCLUDED__ */
