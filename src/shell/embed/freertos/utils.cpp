/*
 *  Shell library
 *  Utility functions (freertos)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.03.2017 16:08:32
 *      Initial revision.
 */

#include <FreeRTOS.h>
#include <task.h>

#include "shell/shell.h"
#include "shell/utils.h"

/*
 * Delay execution of the current thread
 *
 *      nSecond     seconds to delay
 */
void sleep_s(unsigned int nSecond)
{
    vTaskDelay((TickType_t)nSecond*1000);
}

/*
 * Delay execution of the current thread
 *
 *      nMillisecond        milliseconds to delay
 */
void sleep_ms(unsigned int nMillisecond)
{
    vTaskDelay((TickType_t)nMillisecond);
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
    vTaskDelay(1);
}
