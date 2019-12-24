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
 *  Revision 1.0, 13.03.2017 11:53:12
 *      Initial revision.
 *
 */

#include "tn.h"

#include "shell/config.h"
#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/utils.h"
#include "shell/static_allocator.h"
#include "shell/thread.h"

#include "shell/embed/tneo/tn_backend.h"


#define _STACK_SIZE			ALIGN(CARBON_THREAD_STACK_SIZE, 8/4)
#define _STACK_SIZE_BYTES	(_STACK_SIZE*sizeof(TN_UWord))

TN_STACK_ARR_DEF(stacks, _STACK_SIZE*CARBON_THREAD_COUNT);

static CStaticAllocator<_STACK_SIZE_BYTES, 8>
	stackAllocator(stacks, _STACK_SIZE_BYTES*CARBON_THREAD_COUNT);

CThread::CThread(const char* strName, hr_time_t hrStartTimeout, hr_time_t hrStopTimeout) :
	m_pStack(0),
	m_state(THREAD_STATE_NONE),
	m_cbThread(0),
	m_pThreadData(0),
	m_nResult(ENOENT),
	m_hrStartTimeout(hrStartTimeout),
	m_hrStopTimeout(hrStopTimeout)
{
	copyString(m_strName, strName, sizeof(m_strName));
	_tbzero_object(m_task);
}

CThread::CThread(hr_time_t hrStartTimeout, hr_time_t hrStopTimeout) :
	m_pStack(0),
	m_state(THREAD_STATE_NONE),
	m_cbThread(0),
	m_pThreadData(0),
	m_nResult(ENOENT),
	m_hrStartTimeout(hrStartTimeout),
	m_hrStopTimeout(hrStopTimeout)
{
	m_strName[0] = '\0';
	_tbzero_object(m_task);
}

CThread::~CThread()
{
}

result_t CThread::start(thread_cb_t cb, void* pData)
{
	enum TN_RCode	rc;
	result_t		nresult = ESUCCESS;

	shell_assert(m_state == THREAD_STATE_NONE);
	shell_assert(m_pStack == 0);

	m_pStack = stackAllocator.alloc();
	if ( !m_pStack )  {
		log_error(L_GEN, "[thread] stack allocation failure\n");
		return ENOMEM;
	}

	m_cbThread = cb;
	m_pThreadData = pData;
	m_nResult = ETIMEDOUT;

	m_bootComplete.lock();
	rc = tn_task_create(&m_task, threadEntryST, CARBON_THREAD_PRIORITY,
						(TN_UWord*)m_pStack, _STACK_SIZE,
						this, TN_TASK_CREATE_OPT_START);
	if ( rc != TN_RC_OK )  {
		/*
		 * Start thread failed
		 */
		nresult = errorTn2Shell(rc);
		m_bootComplete.unlock();

		log_error(L_GEN, "[thread] %s: failed to create thread, result: %d (%d)\n",
				  m_strName, nresult, rc);

		stackAllocator.dealloc(m_pStack);
		m_pStack = 0;

		return nresult;
	}

	m_state = THREAD_STATE_RUNNING;

	if ( m_hrStartTimeout > 0 )  {
		/*
		 * Waiting for the thread bootstrap complete
		 */
		nresult = m_bootComplete.waitTimed(hr_time_now() + m_hrStartTimeout);
		m_bootComplete.unlock();

		if ( nresult == ESUCCESS )  {
			if ( m_nResult != ESUCCESS )  {
				log_error(L_GEN, "[thread] %s: function failed, result: %d\n",
						  m_strName, m_nResult);
				nresult = m_nResult;
				stop();
			}
		}
		else  {
			log_error(L_GEN, "[thread] %s: bootstrap failed, result: %d\n",
					  m_strName, nresult);
			stop();
		}
	}
	else  {
		m_bootComplete.unlock();
	}

	return nresult;
}

/*
 * Stop a thread
 */
void CThread::stop()
{
	if ( m_state != THREAD_STATE_NONE ) {
		/*
		 * Stopping a running thread
		 */
		m_state = THREAD_STATE_STOPPING;

		//log_debug(L_GEN, "[thread] %s: stopping thread\n", m_strName);
		if ( m_hrStopTimeout )  {
			int     i;
			enum TN_RCode		rc = TN_RC_OK;
			enum TN_TaskState	state = (enum TN_TaskState)0;
			int     ms = (int)HR_TIME_TO_MILLISECONDS(m_hrStopTimeout);
			int     ms100 = ms < 100 ? 1 : (ms/100);

			for(i=0; i<ms100 && rc == TN_RC_OK; i++)  {
				tn_task_wakeup(&m_task);
				rc = tn_task_state_get(&m_task, &state);
				if ( rc == TN_RC_OK && (state&TN_TASK_STATE_DORMANT) != 0 )  {
					break;
				}
				tn_task_sleep(HR_100MSEC);
			}

			if ( (state&TN_TASK_STATE_DORMANT) != 0 )  {
				log_debug(L_GEN, "[thread] %s: was not stopped, terminate it\n", m_strName);
				tn_task_terminate(&m_task);
			}

			tn_task_delete(&m_task);
		}
		else  {
			/*
			 * Stop thread unconditional
			 */
			//log_dump("[thread] %s: stopping thread unconditional\n", m_strName);

			tn_task_terminate(&m_task);
			tn_task_delete(&m_task);
		}

		m_state = THREAD_STATE_NONE;
		//log_dump("[thread] %s: thread stopped\n", m_strName);
	}

	if ( m_pStack ) {
		stackAllocator.dealloc(m_pStack);
		m_pStack = 0;
	}
}
