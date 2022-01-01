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
 *  Revision 1.0, 27.03.2013 16:24:13
 *      Initial revision.
 *
 *  Revision 2.0, 18.07.2015 22:44:51
 *  	Completely rewrite to use non-static callbacks.
 */

#ifndef __CARBON_EVENT_TIMER_H_INCLUDED__
#define __CARBON_EVENT_TIMER_H_INCLUDED__

#include <functional>

#include "shell/config.h"
#include "shell/types.h"
#include "shell/hr_time.h"
#include "shell/object.h"
#include "shell/lockedlist.h"

#if CARBON_DEBUG_TRACK_OBJECT
#include "shell/track_object.h"
#endif /* CARBON_DEBUG_TRACK_OBJECT */

typedef std::function<void(void*)> timer_cb_t;

#if CARBON_DEBUG_TRACK_OBJECT
#define __CTimer_PARENT				,public CTrackObject<CTimer>
#else /* CARBON_DEBUG_TRACK_OBJECT */
#define __CTimer_PARENT
#endif /* CARBON_DEBUG_TRACK_OBJECT */

class CTimer : public CObject, public CListItem __CTimer_PARENT
{
    public:
        enum {
            timerPeriodic = 0x1
        };

    protected:
        hr_time_t           m_hrTime;             	/* Next fire time */
        hr_time_t           m_hrPeriod;           	/* Timer period */
        timer_cb_t          m_callback;         	/* Timer function */
        int                 m_options;              /* Timer options, timerXXX */
        void*               m_pParam;           	/* Timer parameter */

    public:
        CTimer(hr_time_t hrPeriod, timer_cb_t callback, int options, void* pParam, const char* strName);
        CTimer(hr_time_t hrPeriod, timer_cb_t callback, int options, const char* strName);
        CTimer(hr_time_t hrPeriod, timer_cb_t callback, void* pParam, const char* strName);
        CTimer(hr_time_t hrPeriod, timer_cb_t callback, const char* strName);
        virtual ~CTimer();

		void* operator new(size_t size) throw();
		void* operator new[](size_t size) throw();
		void operator delete(void* pData);
		void operator delete[](void* pData);

    public:
        boolean_t isPeriodic() const { return (m_options&timerPeriodic) != 0; }
        hr_time_t getTime() const { return m_hrTime; }

        virtual void restart(hr_time_t hrNewPeriod = HR_0) {
        	if ( hrNewPeriod != HR_0 )  { m_hrPeriod = hrNewPeriod; }
            m_hrTime = hr_time_now() + m_hrPeriod;
        }

        virtual void pause() {
            m_hrTime = hr_time_now() + HR_FOREVER;
        }

        virtual void execute() {
            if ( m_callback )  {
                m_callback(m_pParam);
        	}
    	}

	public:
#if CARBON_DEBUG_DUMP
        virtual void dump(const char* strPref = "") const;
#else /* CARBON_DEBUG_DUMP */
		virtual void dump(const char* strPref = "") const { UNUSED(strPref); }
#endif /* CARBON_DEBUG_DUMP */
};

#define SAFE_DELETE_TIMER(__pTimer, __pEventLoop)		\
	do { \
		if ( (__pTimer) != 0 ) { \
			(__pEventLoop)->deleteTimer(__pTimer); \
			(__pTimer) = 0; \
		} \
	} while(0)

#define TIMER_CALLBACK(__class_member, __object)    \
    std::bind(&__class_member, __object, std::placeholders::_1)

/*
 * WatchDog API
 */
typedef CTimer*		wd_handle_t;

class CEventLoop;

extern wd_handle_t wdCreate(hr_time_t hrTimeout, timer_cb_t cb, CEventLoop* pEventLoop = 0,
					 void* p = 0, const char* strName = 0);
extern void wdCancel(wd_handle_t wdHandle, CEventLoop* pEventLoop = 0);

#define SAFE_CANCEL_WD_WITH_EVENTLOOP(__handle, __pEventLoop)	\
	do { \
		if ( (__handle) != 0 ) { \
			wdCancel(__handle, __pEventLoop); \
			(__handle) = 0; \
		} \
	} while(0)

#define SAFE_CANCEL_WD(__handle)	\
	do { \
		if ( (__handle) != 0 ) { \
            wdCancel(__handle); \
            (__handle) = 0; \
        } \
	} while(0)


#endif /* __CARBON_EVENT_TIMER_H_INCLUDED__ */
