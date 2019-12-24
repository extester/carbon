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
 *  Revision 1.0, 13.03.2017, 10:51:28
 *      Initial revision.
 */

#ifndef __SHELL_HR_TIME_INCLUDED__
#define __SHELL_HR_TIME_INCLUDED__

#include "shell/types.h"
#include "shell/assert.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef int64_t     			hr_time_t;

/*
 * Time resolution: 1 usec
 */
#define HR_TIME_RESOLUTION		((hr_time_t)1000)


#define HR_TIME_TO_MICROSECONDS(__hrtime)		((int64_t)(__hrtime)*1000)
#define HR_TIME_TO_MILLISECONDS(__hrtime)		((int64_t)(__hrtime))
#define HR_TIME_TO_SECONDS(__hrtime)			((int64_t)((__hrtime)/HR_TIME_RESOLUTION))

#define SECONDS_TO_HR_TIME(__sec)               ((hr_time_t)((__sec)*HR_TIME_RESOLUTION))
#define MILLISECONDS_TO_HR_TIME(__msec)			((hr_time_t)((hr_time_t)(__msec)))
#define MICROSECONDS_TO_HR_TIME(__mcsec)		((hr_time_t)(__mcsec)/1000)

#define HR_TIME_TO_TN(__hrtime)					((TN_TickCnt)(__hrtime))
#define TN_TO_HR_TIME(__tick)					((hr_time_t)(__tick))

/*
 * Get current time
 *
 * Return: current time
 */
extern hr_time_t hr_time_now();

/*
 * Calculate elapsed time interval
 *
 * 		hrStart		start interval time
 *
 * Return: elapsed time
 */
static inline hr_time_t hr_time_get_elapsed(hr_time_t hrStart)
{
	return hr_time_now() - hrStart;
}

/*
 * Calculate a rest timeout
 *
 * 		hrStart			started timestamp
 * 		hrTimeout		maximum timeout from hrStart
 *
 * Return: rest time (may be HR_0)
 */
static inline hr_time_t hr_timeout(hr_time_t hrStart, hr_time_t hrTimeout)
{
	hr_time_t hrNow = hr_time_now();
	return (hrStart+hrTimeout) > hrNow ? ((hrStart+hrTimeout)-hrNow) : ((hr_time_t)0);
}


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __SHELL_HR_TIME_INCLUDED__ */
