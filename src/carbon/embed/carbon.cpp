/*
 *  Carbon framework
 *  Library definitions, types and includes (STM32)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 16.02.2017 13:00:27
 *      Initial revision.
 */

#include "carbon/carbon.h"

/*
 * Carbon library bootstrap initialisation
 */
void carbon_init()
{
    /* Initialise logger including STDOUT appender, all levels/channels are enabled */
    logger_init(TRUE);
    logger_enable(LT_ALL|L_ALL);
}

/*
 * Carbon library termination
 */
void carbon_terminate()
{
    logger_terminate();
}
