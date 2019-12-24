/*
 *  Shell library
 *  Synchronization primitives (tneo)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.03.2017 10:57:09
 *      Initial revision.
 */

#ifndef __SHELL_LOCK_H_INCLUDED__
#define __SHELL_LOCK_H_INCLUDED__

#include "tn_mutex.h"
#include "tn_tasks.h"
#include "tn_sem.h"

#include "shell/assert.h"
#include "shell/hr_time.h"
#include "shell/error.h"
#include "shell/tstring.h"
#include "shell/logger.h"

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
		TN_Mutex    m_mutex;

    public:
        typedef enum {
            mutexNormal,
            mutexRecursive,
            mutexErrorCheck
        } MutexType;

    public:
        CMutex(MutexType mutexType = mutexNormal) : CLock()
        {
            if ( mutexType == mutexNormal )  {
                /* Default mutex type */
            	// TODO set max available thread priority
                enum TN_RCode   rc;

				_tbzero_object(m_mutex);
            	rc = tn_mutex_create(&m_mutex, TN_MUTEX_PROT_CEILING, 6/*5*/ /*254*/);
                if ( rc != TN_RC_OK ) {
                    log_error(L_GEN, "[mutex] create error %d\n", rc);
                }
            }
            else  {
                /* Create specified type mutex */
                shell_assert(FALSE); /*TODO*/
            }
        }

        virtual ~CMutex()
        {
			enum TN_RCode   rc;

            rc = tn_mutex_delete(&m_mutex);
            if ( rc != TN_RC_OK ) {
                log_error(L_GEN, "[mutex] delete error %d\n", rc);
            }
        }

    public:
        virtual void lock()
        {
			enum TN_RCode   rc;

        	// TODO set work timeout to constant
        	rc =  tn_mutex_lock(&m_mutex, 10000000);
			if ( rc != TN_RC_OK ) {
				log_error(L_GEN, "[mutex] lock() error %d\n", rc);
			}
        }

        virtual void unlock()
        {
			enum TN_RCode   rc;

        	rc = tn_mutex_unlock(&m_mutex);
			if ( rc != TN_RC_OK ) {
				log_error(L_GEN, "[mutex] unlock() error %d\n", rc);
			}
        }

        virtual result_t trylock()
        {
			enum TN_RCode   rc;

        	// TODO set work timeout to constant
            rc = tn_mutex_lock(&m_mutex, 10000000);

            return rc == TN_RC_OK ? ESUCCESS : ETIMEDOUT;
        }
};


/*
 * Conditional variable
 */
class CCondition : public CMutex
{
    protected:
		struct TN_Sem	m_semaphore;
		int				m_waitCount;


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
        result_t waitTimed(hr_time_t hrTime);

        /*
	     * Wait unconditional
         *
	     * Return: ESUCCESS, ETIMEDOUT
	     */
        result_t wait();

		/*
		 * Signal conditional variable to wakeup
		 */
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
