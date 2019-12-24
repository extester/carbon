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
 *  Revision 1.0, 24.03.2017 10:09:54
 *      Initial revision.
 */

#include "shell/lock.h"


CCondition::CCondition() :
    CMutex(),
    m_waitCount(0)
{
    m_hSemaphore = xSemaphoreCreateCountingStatic(0xffffffff, 0, &m_semaphore);
    shell_assert(m_hSemaphore != NULL);
}

CCondition::~CCondition()
{
    if ( m_hSemaphore ) {
        vSemaphoreDelete(m_hSemaphore);
    }
}

result_t CCondition::waitTimed(hr_time_t hrTime)
{
    hr_time_t		hrNow, hrTimeout;
    TickType_t      ticks;
    BaseType_t      rc;

    hrNow = hr_time_now();
    if ( hrNow >= hrTime )  {
        return ETIMEDOUT;
    }

    hrTimeout = hrTime-hrNow;
    ticks = hrTimeout < 0xffffffff ? HR_TIME_TO_TICK(hrTimeout) : 0xffffffff;

    vPortEnterCritical();
    unlock();

    m_waitCount++;
    vPortExitCritical();
    rc = xSemaphoreTake(m_hSemaphore, ticks);
    vPortEnterCritical();
    m_waitCount--;

    lock();
    vPortExitCritical();

    return rc == pdTRUE ? ESUCCESS : ETIMEDOUT;
}

result_t CCondition::wait()
{
    return waitTimed(HR_FOREVER);
}

void CCondition::wakeup()
{
    int     i, count;

    vPortEnterCritical();

    count = m_waitCount;
    for(i=0; i<count; i++)  {
        shell_verify(xSemaphoreGive(m_hSemaphore) == pdTRUE);
    }

    vPortExitCritical();
}

