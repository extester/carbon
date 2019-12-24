/*
 *  Shell library
 *  High resolution time (tneo)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.03.2017, 10:54:04
 *      Initial revision.
 */

#include <tn.h>

#include "shell/hr_time.h"

extern "C" void SysTick_Handler(void);

static volatile hr_time_t	g_hrCounter = 0L;


void SysTick_Handler()
{
	g_hrCounter++;
	tn_tick_int_processing();
}

/*
 * Get current time
 *
 * Return: current time
 */
hr_time_t hr_time_now()
{
	hr_time_t		hrTime;
	int 			n = 0;

	do {
		hrTime = g_hrCounter;
	} while ( hrTime != g_hrCounter && n++ < 50 );

	return hrTime;
}
