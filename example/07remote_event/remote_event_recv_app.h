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
 *	Revision 1.0, 03.08.2016 16:51:17
 *	    Initial revision.
 */

#ifndef __EXAMPLE_REMOTE_EVENT_RECV_APP_H_INCLUDED__
#define __EXAMPLE_REMOTE_EVENT_RECV_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/event/remote_event_service.h"

class CRemoteEventRecvApp : public CApplication
{
	private:
		int		m_nEventCount;		/* Processed remote event count */

    public:
		CRemoteEventRecvApp(int argc, char* argv[]);
        virtual ~CRemoteEventRecvApp();

	public:
        virtual result_t init();
        virtual void terminate();

    protected:
        virtual boolean_t processEvent(CEvent* pEvent);
		virtual void initEvent();

	private:
		void dumpEvent(CRemoteEvent* pEvent) const;
		void replyEvent(CRemoteEvent* pEvent);
};


#endif /* __EXAMPLE_REMOTE_EVENT_RECV_APP_H_INCLUDED__ */
