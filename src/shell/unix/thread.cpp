/*
 *  Shell library
 *  Worker thread (UNIX)
 *
 *  Copyright (c) 2013-2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.03.2013 18:32:04
 *      Initial revision.
 *
 *  Revision 1.1, 13.06.2015 23:23:00
 *      Added m_sigBlockSet, getName().
 *
 *  Revision 1.2, 28.06.2016 14:01:21
 *      Use SIGINT for thread termination.
 *
 *  Revision 1.3, 01.11.2016 16:31:15
 *      Use SIGTERM for thread termination.
 *
 */

#include <signal.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/utils.h"
#include "shell/thread.h"

/*******************************************************************************
 * CThread class
 */

CThread::CThread(const char* strName, hr_time_t hrStartTimeout, hr_time_t hrStopTimeout) :
    m_id(0),
    m_cbThread(0),
    m_pThreadData(0),
    m_nResult(ENOENT),
    m_hrStartTimeout(hrStartTimeout),
    m_hrStopTimeout(hrStopTimeout),
    m_bStopping(FALSE)
{
    copyString(m_strName, strName, sizeof(m_strName));

    sigfillset(&m_sigBlockSet);
    sigdelset(&m_sigBlockSet, SIGQUIT);
    sigdelset(&m_sigBlockSet, SIGBUS);
    sigdelset(&m_sigBlockSet, SIGFPE);
    sigdelset(&m_sigBlockSet, SIGILL);
    sigdelset(&m_sigBlockSet, SIGSEGV);
}

CThread::CThread(hr_time_t hrStartTimeout, hr_time_t hrStopTimeout) :
    m_id(0),

    m_cbThread(0),
    m_pThreadData(0),
    m_nResult(ENOENT),
    m_hrStartTimeout(hrStartTimeout),
    m_hrStopTimeout(hrStopTimeout),
    m_bStopping(FALSE)
{
    m_strName[0] = '\0';

    sigfillset(&m_sigBlockSet);
    sigdelset(&m_sigBlockSet, SIGQUIT);
    sigdelset(&m_sigBlockSet, SIGBUS);
    sigdelset(&m_sigBlockSet, SIGFPE);
    sigdelset(&m_sigBlockSet, SIGILL);
    sigdelset(&m_sigBlockSet, SIGSEGV);
}

CThread::~CThread()
{
}

/*
 * Setup a visible thread name
 *
 * Return: ESUCCESS, ...
 */
result_t CThread::setName()
{
	int			retVal;
	result_t	nresult = ESUCCESS;

	retVal = prctl(PR_SET_NAME, m_strName, 0, 0, 0);
	if ( retVal != 0 )  {
		nresult = errno;
	}

	return nresult;
}

/*
 * Start a thread
 *
 *      cdThread    thread function
 *      pData       user data pointer passed to the thread
 *
 * Return: ESUCCESS, ETIMEDOUT, ...
 */
result_t CThread::start(thread_cb_t cbThread, void* pData)
{
    pthread_attr_t      thread_attr;
    result_t			nresult;

    shell_assert(m_id == 0);

    shell_verify(pthread_attr_init(&thread_attr) == 0);

    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);

    m_cbThread = cbThread;
    m_pThreadData = pData;
    m_nResult = ETIMEDOUT;

    m_bootComplete.lock();
    nresult = pthread_create(&m_id, &thread_attr, threadEntryST, this);
    if ( nresult != ESUCCESS ) {
        /*
         * Start thread failed
         */
        nresult = errno;
        m_bootComplete.unlock();

        log_error(L_GEN, "[thread] %s: failed to create thread, result: %d\n",
                                m_strName, nresult);
        pthread_attr_destroy(&thread_attr);
        return nresult;
    }

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

    shell_verify(pthread_attr_destroy(&thread_attr) == 0);

    return nresult;
}

/*
 * Stop a thread
 */
void CThread::stop()
{
    m_bStopping = TRUE;

    if ( isRunning() )  {
        /*
         * Stopping a running thread
         */
        //log_debug(L_GEN, "[thread] %s: stopping thread\n", m_strName);
        if ( m_hrStopTimeout )  {
            int     i, retVal;
            int     ms = (int)HR_TIME_TO_MILLISECONDS(m_hrStopTimeout);
            int     ms100 = ms < 100 ? 1 : (ms/100);

            retVal = pthread_kill(m_id, SIGQUIT);

            for(i=0; i<ms100 && retVal == 0; i++)  {
            	retVal = pthread_kill(m_id, SIGQUIT);
                usleep(100000);
            }

            if ( retVal != 0 )  {
                pthread_join(m_id, NULL);
            }
            else  {
            	log_debug(L_GEN, "[thread] %s: was not killed, cancel it\n", m_strName);

                pthread_cancel(m_id);
                pthread_join(m_id, NULL);
            }
        }
        else  {
            /*
             * Stop thread unconditional
             */
            log_dump("[thread] %s: stopping thread unconditional\n", m_strName);

            pthread_kill(m_id, SIGQUIT);
            pthread_join(m_id, NULL);
        }

		m_id = 0;
        //log_dump("[thread] %s: thread stopped\n", m_strName);
    }
    m_bStopping = FALSE;
}

/*
 * A thread's common initialisation code
 *
 * Return: thread exit code
 */
void* CThread::threadEntry()
{
    sigset_t        sigset;

    /* Setup thread name */
    setName();

    /* Block signals accroding mask for the current thread */
    sigfillset(&sigset);
    pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
    pthread_sigmask(SIG_BLOCK, &m_sigBlockSet, NULL);

    return this->m_cbThread(this, m_pThreadData);
}
