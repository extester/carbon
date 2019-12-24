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
 *  Revision 1.0, 03.03.2017 13:08:43
 *      Initial revision.
 */

#include "shell/lock.h"

/*******************************************************************************
 * CCondition
 */

CCondition::CCondition() :
    CMutex(),
    m_waitCount(0)
{
    enum TN_RCode   rc;

	_tbzero_object(m_semaphore);
    rc = tn_sem_create(&m_semaphore, 0, 0xffff);
	if ( rc != TN_RC_OK )  {
		log_error(L_GEN, "[cond] create semaphore error %d\n", rc);
	}
}

CCondition::~CCondition()
{
	enum TN_RCode   rc;

    rc = tn_sem_delete(&m_semaphore);
	if ( rc != TN_RC_OK )  {
		log_error(L_GEN, "[cond] delete semaphore error %d\n", rc);
	}
}

/*
 * Wait for condition variable
 *
 *      hrTime       	time to stop waiting
 *
 * Return:
 *      ESUCCESS        conditional variable has been signaled
 *      ETIMEDOUT       timeout expired without signal
 */
result_t CCondition::waitTimed(hr_time_t hrTime)
{
    enum TN_RCode   rc;
    TN_TickCnt      ticks;
	hr_time_t		hrNow, hrTimeout;
   	TN_INTSAVE_DATA;

	hrNow = hr_time_now();
	if ( hrNow >= hrTime )  {
		return ETIMEDOUT;
	}

	hrTimeout = hrTime-hrNow;
    ticks = hrTimeout < 0xffffffff ? HR_TIME_TO_TN(hrTimeout) : 0xffffffff;

    TN_INT_DIS_SAVE();      //-- disable interrupts
    unlock();

    m_waitCount++;
    TN_INT_RESTORE();
    rc = tn_sem_wait(&m_semaphore, ticks);
	TN_INT_DIS_SAVE();      //-- disable interrupts
    m_waitCount--;

    lock();
    TN_INT_RESTORE();

	return rc == TN_RC_OK ? ESUCCESS : ETIMEDOUT;
}

/*
 * Wait unconditional
 *
 * Return: ESUCCESS, ...
 */
result_t CCondition::wait()
{
    return waitTimed(HR_FOREVER);
}

/*
 * Signal conditional variable to wakeup
 */
void CCondition::wakeup()
{
    int     i, count;
    TN_INTSAVE_DATA;

    TN_INT_DIS_SAVE();

    count = m_waitCount;
    for(i=0; i<count; i++)  {
        tn_sem_signal(&m_semaphore);
    }

    TN_INT_RESTORE();
}
