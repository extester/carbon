/*
 *  Shell library
 *  Synchronization primitives (UNIX)
 *
 *  Copyright (c) 2013-2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 27.03.2013 11:30:57
 *      Initial revision.
 *
 *  Revision 1.1, 27.01.2017 11:58:59
 *      Added enum MutexType.
 */

#ifndef __SHELL_LOCK_H_INCLUDED__
#define __SHELL_LOCK_H_INCLUDED__

#include <pthread.h>

#include "shell/assert.h"
#include "shell/hr_time.h"
#include "shell/error.h"

/*
 * Virtual base class for the all synchronization primitives
 */
class CLock
{
    protected:
        CLock() {}
        virtual ~CLock() {}

    public:
        virtual void lock() = 0;
        virtual void unlock() = 0;
};

/*
 * Mutex class
 */
class CMutex : public CLock
{
    protected:
        pthread_mutex_t m_mutex;

    public:
        enum MutexType {
            mutexNormal = PTHREAD_MUTEX_NORMAL,
            mutexRecursive = PTHREAD_MUTEX_RECURSIVE,
            mutexErrorCheck = PTHREAD_MUTEX_ERRORCHECK
        };

    public:
        CMutex(MutexType mutexType = mutexNormal) : CLock()
        {
            if ( mutexType == mutexNormal )  {
                /* Default mutex type */
                shell_verify(pthread_mutex_init(&m_mutex, NULL) == 0);
            }
            else  {
                /* Create specified type mutex */
                pthread_mutexattr_t     attr;

                shell_verify(pthread_mutexattr_init(&attr) == 0);
                shell_verify(pthread_mutexattr_settype(&attr, (int)mutexType) == 0);
                shell_verify(pthread_mutex_init(&m_mutex, &attr) == 0);
                shell_verify(pthread_mutexattr_destroy(&attr) == 0);
            }
        }

        virtual ~CMutex()
        {
            shell_verify(pthread_mutex_destroy(&m_mutex) == 0);
        }

    public:
        virtual void lock()
        {
            shell_verify(pthread_mutex_lock(&m_mutex) == 0);
        }

        virtual void unlock()
        {
            shell_verify(pthread_mutex_unlock(&m_mutex) == 0);
        }

        virtual result_t trylock()
        {
            int     retVal;

            retVal = pthread_mutex_trylock(&m_mutex);

            return retVal == 0 ? ESUCCESS : (result_t)retVal;
        }
};

/*
 * Conditional variable
 */
class CCondition : public CMutex
{
    protected:
        pthread_cond_t m_cond;

    public:
        CCondition() : CMutex()
        {
            pthread_condattr_t     cond_attr;

            shell_verify(pthread_condattr_init(&cond_attr) == 0);
            shell_verify(pthread_condattr_setclock(&cond_attr, HR_TIME_CLOCKID) == 0);

            shell_verify(pthread_cond_init(&m_cond, &cond_attr) == 0);
            shell_verify(pthread_condattr_destroy(&cond_attr) == 0);
        }

        virtual ~CCondition()
        {
            shell_verify(pthread_cond_destroy(&m_cond) == 0);
        }

    public:

        /*
	     * Wait for a event
	     *
	     *      time        time to stop waiting
	     *
	     * Return: ESUCCESS, ETIMEDOUT, ...
	     *
	     * Note: mutex must be held
	     */
        result_t waitTimed(hr_time_t hrTimeout)
        {
            struct timespec     ts;
            int					retVal;

            hr_time_get_timespec(&ts, hrTimeout);
            retVal = pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
            return retVal == 0 ? ESUCCESS : (result_t)retVal;
        }

        /*
	     * Wait unconditional
         *
         * Return: ESUCCESS, ...
	     */
        result_t wait()
        {
            int     retVal;

            retVal = pthread_cond_wait(&m_cond, &m_mutex);
            return retVal == 0 ? ESUCCESS : (result_t)retVal;
        }

        void wakeup()
        {
            pthread_cond_broadcast(&m_cond);
        }
};

/*
 * Automatically lock/unlock helper
 */
class CAutoLock
{
    private:
        CLock&              m_lock;
        mutable boolean_t   m_bLocked;

    public:
        CAutoLock(CLock& rlock) : m_lock(rlock), m_bLocked(FALSE)
        {
            lock();
        }

        ~CAutoLock()
        {
            unlock();
        }

    public:
        void lock() const
        {
            if ( !m_bLocked )  {
                m_lock.lock();
                m_bLocked = TRUE;
            }
        }

        void unlock() const
        {
            if ( m_bLocked )  {
                m_lock.unlock();
                m_bLocked = FALSE;
            }
        }
};

#endif /* __SHELL_LOCKS_H_INCLUDED__ */
