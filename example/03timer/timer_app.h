/*
 *	Carbon Framework Examples
 *	Timer application
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 01.06.2016 15:33:43
 *	    Initial revision.
 */

#ifndef __EXAMPLE_TIMER_APP_H_INCLUDED__
#define __EXAMPLE_TIMER_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/timer.h"

#include "receiver.h"
#include "timer_app.h"

/*
 * User defined custom event
 */
#define EV_TEST                 (EV_USER+0)

class CTimerApp : public CApplication
{
    protected:
        CReceiverModule*    m_pReceiver;
        CTimer*             m_pTimer;

    public:
        CTimerApp(int argc, char* argv[]);
        virtual ~CTimerApp();

	public:
        virtual result_t init();
        virtual void terminate();

    private:
        void startTimer();
        void stopTimer();
        void timerHandler(void* p);

};


#endif /* __EXAMPLE_TIMER_APP_H_INCLUDED__ */
