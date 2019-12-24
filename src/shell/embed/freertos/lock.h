/*
 *  Shell library
 *  Synchronization primitives (freertos)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 24.03.2017 09:34:59
 *      Initial revision.
 */

#ifndef __SHELL_LOCK_H_INCLUDED__
#define __SHELL_LOCK_H_INCLUDED__

#include <FreeRTOS.h>
#include <semphr.h>

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
        StaticSemaphore_t       m_semaphore;
        SemaphoreHandle_t       m_hSemaphore;

    public:
        typedef enum {
            mutexNormal,
            mutexRecursive,
            mutexErrorCheck
        } MutexType;

    public:
        CMutex(MutexType mutexType = mutexNormal) : CLock(), m_hSemaphore(0)
        {
            if ( mutexType == mutexNormal )  {
                /* Default mutex type */
                shell_verify(m_hSemaphore = xSemaphoreCreateMutexStatic(&m_semaphore));
            }
            else  {
                shell_assert(FALSE);
            }
        }

        virtual ~CMutex()
        {
            if ( m_hSemaphore ) {
                vSemaphoreDelete(m_hSemaphore);
            }
        }

    public:
        virtual void lock()
        {
            shell_verify(xSemaphoreTake(m_hSemaphore, portMAX_DELAY) == pdTRUE);
        }

        virtual void unlock()
        {
            xSemaphoreGive(m_hSemaphore);
        }

        virtual result_t trylock()
        {
            BaseType_t  pd;

            pd = xSemaphoreTake(m_hSemaphore, (TickType_t)0);
            return pd == pdTRUE ? ESUCCESS : ETIMEDOUT;
        }
};

/*
 * Conditional variable
 */
class CCondition : public CMutex
{
    protected:
        StaticSemaphore_t       m_semaphore;
        SemaphoreHandle_t       m_hSemaphore;
        int				        m_waitCount;

    public:
        CCondition();
        virtual ~CCondition();

    public:

        /*
	     * Wait for a event
	     *
	     *      time        time to stop waiting
	     *
	     * Return: ESUCCESS, ETIMEDOUT
	     *
	     * Note: mutex must be held
	     */
        result_t waitTimed(hr_time_t hrTimeout);

        /*
	     * Wait unconditional
	     *
	     * Return: ESUCCESS, ...
	     */
        result_t wait();

        void wakeup();
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
