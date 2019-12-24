/*
 *  Shell library
 *  Worker thread (tneo)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.03.2017 11:20:12
 *      Initial revision.
 */

#ifndef __SHELL_THREAD_H_INCLUDED__
#define __SHELL_THREAD_H_INCLUDED__

#include <functional>

#include "tn_tasks.h"

#include "shell/config.h"
#include "shell/types.h"
#include "shell/lock.h"
#include "shell/hr_time.h"
#include "shell/tstring.h"
#include "shell/embed/functor_allocator.h"

#define THREAD_START_DEFAULT_TIMEOUT        HR_4SEC
#define THREAD_STOP_DEFAULT_TIMEOUT         HR_8SEC

class CThread;
typedef std::function<void*(CThread*, void*)> thread_cb_t;

class CThread
{
	protected:
		typedef enum {
			THREAD_STATE_NONE,
			THREAD_STATE_RUNNING,
			THREAD_STATE_STOPPING
		} thread_state_t;

		char                    m_strName[CARBON_OBJECT_NAME_LENGTH];      /* Thread name */
		TN_Task					m_task;
		void*					m_pStack;
		volatile thread_state_t	m_state;
		CCondition              m_bootComplete;     /* Bootstrap complete notify */

		thread_cb_t				m_cbThread;		    /* Thread function */
		void*                   m_pThreadData;      /* User data pointer */
		result_t				m_nResult;          /* Boot result code */

		hr_time_t               m_hrStartTimeout;	/* Time to bootstrap, 0 - do not wait */
		hr_time_t               m_hrStopTimeout;	/* Time to shutdown, 0 - do not wait */


	public:
		CThread(const char* strName, hr_time_t hrStartTimeout = THREAD_START_DEFAULT_TIMEOUT,
			hr_time_t hrStopTimeout = THREAD_STOP_DEFAULT_TIMEOUT);

		CThread(hr_time_t hrStartTimeout = THREAD_START_DEFAULT_TIMEOUT,
			hr_time_t hrStopTimeout = THREAD_STOP_DEFAULT_TIMEOUT);

		virtual ~CThread();

	public:
		result_t start(thread_cb_t cb, void* pData = NULL);
		virtual void stop();

		boolean_t isRunning() const { return m_state == THREAD_STATE_RUNNING || m_state == THREAD_STATE_STOPPING; }
		boolean_t isStopping() const { return m_state == THREAD_STATE_STOPPING; }
		boolean_t isAlive() const { return isRunning() && !isStopping(); }

		void* getData() const { return m_pThreadData; }
		const char* getName() { return m_strName; }
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
		static void threadEntryST(void* p)  {
			static_cast<CThread*>(p)->threadEntry();
		}

		void* threadEntry() {
			m_cbThread(this, m_pThreadData);
			return NULL;
		}
};

#define THREAD_CALLBACK(__class_member, __object)    \
    std::bind(&__class_member, __object, std::placeholders::_1, std::placeholders::_2)


#endif /* __SHELL_THREAD_H_INCLUDED__ */
