/*
 *  Carbon framework
 *  Timers base class
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 27.03.2013 16:25:24
 *      Initial revision.
 *
 *  Revision 2.0, 18.07.2015 22:43:01
 *  	Completely rewrite to use non-static callbacks.
 */

#include "carbon/logger.h"
#include "carbon/event/eventloop.h"
#include "carbon/timer.h"

/*
 * Create a timer object with the parameters
 *
 * 		hrPeriod		timer run timeout (or period if CTimer::timerPeriodic)
 * 		callback		timer callback to execute
 * 		options			timer options (timerXXX)
 * 		pParam			timer parameter to pass to callback
 * 		strName			timer name
 */
CTimer::CTimer(hr_time_t hrPeriod, timer_cb_t callback, int options, void* pParam, const char* strName) :
	CObject(strName),
	CListItem(),
	//CTrackObject<CTimer>(),
	m_hrPeriod(hrPeriod),
	m_callback(callback),
	m_options(options),
	m_pParam(pParam)
{
	restart();
}

/*
 * Create a timer object with the parameters
 *
 * 		hrPeriod		timer run timeout (or period if CTimer::timerPeriodic)
 * 		callback		timer callback to execute
 * 		options			timer options (timerXXX)
 * 		strName			timer name
 */
CTimer::CTimer(hr_time_t hrPeriod, timer_cb_t callback, int options, const char* strName) :
	CObject(strName),
	CListItem(),
	//CTrackObject<CTimer>(),
	m_hrPeriod(hrPeriod),
	m_callback(callback),
	m_options(options),
	m_pParam(NULL)
{
	restart();
}

CTimer::CTimer(hr_time_t hrPeriod, timer_cb_t callback, void* pParam, const char* strName) :
	CObject(strName),
	CListItem(),
	//CTrackObject<CTimer>(),
	m_hrPeriod(hrPeriod),
	m_callback(callback),
	m_options(0),
	m_pParam(pParam)
{
	restart();
}

/*
 * Create a timer object with the parameters
 *
 * 		hrPeriod		timer run timeout (or period if CTimer::timerPeriodic)
 * 		callback		timer callback to execute
 * 		strName			timer name
 */
CTimer::CTimer(hr_time_t hrPeriod, timer_cb_t callback, const char* strName) :
	CObject(strName),
	CListItem(),
	//CTrackObject<CTimer>(),
	m_hrPeriod(hrPeriod),
	m_callback(callback),
	m_options(0),
	m_pParam(NULL)
{
	restart();
}

CTimer::~CTimer()
{
}

/*******************************************************************************
 * Debugging support
 */

#if CARBON_DEBUG_DUMP

void CTimer::dump(const char* strPref) const
{
	log_dump("%s%s\n", strPref, getName());
}

#endif /* CARBON_DEBUG_DUMP */

wd_handle_t wdCreate(hr_time_t hrTimeout, timer_cb_t cb, CEventLoop* pEventLoop,
							void* p, const char* strName)
{
	char		tmp[64];
	const char*	s = strName;
	CTimer*		pTimer;

	if ( s == 0 )  {
		snprintf(tmp, sizeof(tmp), "wd (%dms)", (int)HR_TIME_TO_MILLISECONDS(hrTimeout));
		s = tmp;
	}

	pTimer = new CTimer(hrTimeout, cb, 0, p, s);

	if ( pEventLoop )  {
		pEventLoop->insertTimer(pTimer);
	}
	else {
		appMainLoop()->insertTimer(pTimer);
	}

	return pTimer;
}


void wdCancel(wd_handle_t wdHandle, CEventLoop* pEventLoop)
{
	if ( pEventLoop )  {
		pEventLoop->deleteTimer(wdHandle);
	}
	else {
		appMainLoop()->deleteTimer(wdHandle);
	}
}
