/*
 *	Carbon Framework Examples
 *	Worker thread
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 01.06.2016 12:41:16
 *	    Initial revision.
 */

#ifndef __EXAMPLE_THREAD_APP_H_INCLUDED__
#define __EXAMPLE_THREAD_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/timer.h"
#include "carbon/thread.h"
#include "carbon/event.h"

/*
 * User defined custom event
 */
#define EV_TEST                 (EV_USER+0)

class CThreadApp : public CApplication
{
    protected:
        CThread         m_thWorker;

    public:
        CThreadApp(int argc, char* argv[]);
        virtual ~CThreadApp();

	public:
        virtual result_t init();
        virtual void terminate();

    private:
        virtual boolean_t processEvent(CEvent* pEvent);
        void* threadProc(CThread* pThread, void* pData);

};


#endif /* __EXAMPLE_THREAD_APP_H_INCLUDED__ */
