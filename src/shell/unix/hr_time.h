/*
 *  Shell library
 *  High resolution time (UNIX)
 *
 *  Copyright (c) 2013-2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.03.2013, 14:00:58
 *      Initial revision.
 *
 *  Revision 1.1, 30.01.2015
 *  	Added inline function hr_time_now(),
 *  	added macro TIMEVAL_TO_HR_TIME.
 *
 *  Revision 1.2, 30.06.2016 16:18:30
 *  	Added some HR_xxx time constants
 */

#ifndef __SHELL_HR_TIME_INCLUDED__
#define __SHELL_HR_TIME_INCLUDED__

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "shell/types.h"
#include "shell/assert.h"
#include "shell/error.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef int64_t     			hr_time_t;

/*
 * Use clock source for the hr_time_t and pthread_cond_t
 */
#define HR_TIME_CLOCKID         CLOCK_MONOTONIC

/*
 * Time resolution: 1 usec
 */
#define HR_TIME_RESOLUTION		((hr_time_t)1000000)


#define HR_TIME_TO_MICROSECONDS(__hrtime)		((int64_t)(__hrtime))
#define HR_TIME_TO_MILLISECONDS(__hrtime)		((int64_t)((__hrtime)/1000))
#define HR_TIME_TO_SECONDS(__hrtime)			((int64_t)((__hrtime)/HR_TIME_RESOLUTION))

#define SECONDS_TO_HR_TIME(__sec)               ((hr_time_t)((__sec)*HR_TIME_RESOLUTION))
#define MILLISECONDS_TO_HR_TIME(__msec)			((hr_time_t)(((hr_time_t)(__msec))*1000))
#define MICROSECONDS_TO_HR_TIME(__mcsec)		((hr_time_t)(__mcsec))

#define TIMEVAL_TO_HR_TIME(__ptimeval)	\
	((hr_time_t)( (__ptimeval)->tv_sec * HR_TIME_RESOLUTION + (__ptimeval)->tv_usec ))


/*
 * Get current time
 *
 * Return: current time
 */
static inline hr_time_t hr_time_now()
{
    struct timespec ts;

	shell_verify(clock_gettime(HR_TIME_CLOCKID, &ts) == 0);
    return ((hr_time_t)ts.tv_sec) * HR_TIME_RESOLUTION + ts.tv_nsec/1000;
}

/*
 * Convert high resolution time to timeval
 *
 * 		tv			input time in timeval format [out]
 * 		hrTime		time in hr_time format
 */
static inline void hr_time_get_timeval(struct timeval* tv, hr_time_t hrTime)
{
	lldiv_t		q;

	q = lldiv(hrTime, HR_TIME_RESOLUTION);
	tv->tv_sec = (int32_t)q.quot;
	tv->tv_usec = (int32_t)q.rem;
}

/*
 * Convert high resolution time to timespec
 *
 * 		ts			input time in timespec format [out]
 * 		hrTime		time in hr_time format
 */
static inline void hr_time_get_timespec(struct timespec* ts, hr_time_t hrTime)
{
	lldiv_t		q;

	q = lldiv(hrTime, HR_TIME_RESOLUTION);
	ts->tv_sec = (time_t)q.quot;
	ts->tv_nsec = (long int)(q.rem*1000);
}

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

/*
 * Sleep current thread
 *
 * 			hrTimeout		sleep timeout
 *
 * Return: ESUCCESS, EINTR - interrupted by signal
 */
static inline result_t hr_sleep(hr_time_t hrTimeout)
{
	unsigned int	nsec = (unsigned int)HR_TIME_TO_SECONDS(hrTimeout);
	result_t		nresult = ESUCCESS;

	if ( nsec > 0 )  {
		if ( sleep(nsec) != 0 )  {
			return EINTR;
		}
	}

	if ( usleep((__useconds_t)(hrTimeout%SECONDS_TO_HR_TIME(1))) < 0 )  {
		nresult = errno;
	}

	return nresult;
}

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __SHELL_HR_TIME_INCLUDED__ */
