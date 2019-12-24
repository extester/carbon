/*
 *  Shell library
 *  Utility functions (tneo)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.03.2017 11:57:00
 *      Initial revision.
 */

#include "tn_tasks.h"

#include "shell/shell.h"
#include "shell/utils.h"

/*
 * Delay execution of the current thread
 *
 *      nSecond     seconds to delay
 */
void sleep_s(unsigned int nSecond)
{
    tn_task_sleep(nSecond*1000);
}

/*
 * Delay execution of the current thread
 *
 *      nMillisecond        milliseconds to delay
 */
void sleep_ms(unsigned int nMillisecond)
{
    tn_task_sleep(nMillisecond);
}

/*
 * Delay execution of the current thread
 *
 *      nMicrosecond        microseconds to delay
 */
void sleep_us(unsigned int nMicrosecond)
{
    UNUSED(nMicrosecond);

    shell_assert_ex(FALSE, "microseconds delay is not available\n");
    tn_task_sleep(1);
}
