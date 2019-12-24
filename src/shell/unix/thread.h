/*
 *  Shell library
 *  Worker thread (UNIX)
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.03.2013 18:32:19
 *      Initial revision.
 *
 *  Revision 1.1, 13.06.2015 23:23:45
 *      Added m_sigBlockSet, getName().
 *
 */

#ifndef __SHELL_THREAD_H_INCLUDED__
#define __SHELL_THREAD_H_INCLUDED__

#include <functional>
#include <pthread.h>

#include "shell/types.h"
#include "shell/lock.h"
#include "shell/hr_time.h"
#include "shell/object.h"
#include "shell/tstring.h"

#define THREAD_START_DEFAULT_TIMEOUT        HR_4SEC
#define THREAD_STOP_DEFAULT_TIMEOUT         HR_8SEC

class CThread;
typedef std::function<void*(CThread*, void*)> thread_cb_t;

class CThread
{
    protected:
        char                    m_strName[CARBON_OBJECT_NAME_LENGTH];      /* Thread name */
        pthread_t               m_id;               /* Thread ID 0 */
        sigset_t                m_sigBlockSet;      /* Blocking signal set */
        CCondition              m_bootComplete;     /* Bootstrap complete notify */

        thread_cb_t			    m_cbThread;		    /* Thread function */
        void*                   m_pThreadData;      /* User data pointer */
        result_t				m_nResult;          /* Boot result code */

        hr_time_t               m_hrStartTimeout;	/* Time to bootstrap, 0 - do not wait */
        hr_time_t               m_hrStopTimeout;	/* Time to shutdown, 0 - do not wait */
        volatile boolean_t      m_bStopping;		/* TRUE: thread is shutting down */

    public:
        CThread(const char* strName, hr_time_t hrStartTimeout = THREAD_START_DEFAULT_TIMEOUT,
                hr_time_t hrStopTimeout = THREAD_STOP_DEFAULT_TIMEOUT);

        CThread(hr_time_t hrStartTimeout = THREAD_START_DEFAULT_TIMEOUT,
            hr_time_t hrStopTimeout = THREAD_STOP_DEFAULT_TIMEOUT);

        virtual ~CThread();

    public:
        virtual result_t start(thread_cb_t cb, void* pData = NULL);
        virtual void stop();

        boolean_t isRunning() const { return m_id != 0; }
        boolean_t isStopping() const { return m_bStopping; }
        boolean_t isAlive() const { return isRunning() && !isStopping(); }

        void* getData() const { return m_pThreadData; }
        const char* getName() const { return m_strName; }
        void setName(const char* strName) {
            copyString(m_strName, strName, sizeof(m_strName));
        }

        void bootCompleted(int nresult)  {
            m_bootComplete.lock();
            m_nResult = nresult;
            m_bootComplete.wakeup();
            m_bootComplete.unlock();
        }

    private:
        result_t setName();

        static void* threadEntryST(void* p)  {
        	return static_cast<CThread*>(p)->threadEntry();
        }
        void* threadEntry();
};

#define THREAD_CALLBACK(__class_member, __object)    \
    std::bind(&__class_member, __object, std::placeholders::_1, std::placeholders::_2)


#endif /* __SHELL_THREAD_H_INCLUDED__ */
