/*
 *  Carbon framework
 *  Thread event loop
 *
 *  Copyright (c) 2013-2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 27.03.2013 12:21:22
 *      Initial revision.
 *
 *  Revision 1.1, 14.02.2017 13:12:54
 *  	Removed std library lists.
 */

#ifndef __CARBON_EVENTLOOP_H_INCLUDED__
#define __CARBON_EVENTLOOP_H_INCLUDED__

#include "shell/config.h"
#include "shell/object.h"

#include "carbon/thread.h"
#include "carbon/lock.h"
#include "carbon/event.h"
#include "carbon/timer.h"
#include "carbon/utils.h"

#define EVENT_LOOP_ITERATION_TIMEOUT    HR_1MIN

/******************************************************************************
 * Event loop class
 */

class CSyncBase;

class CEventLoop : public CObject
{
    protected:
        boolean_t           		m_bDone;                /* Global EXIT flag */
        CCondition          		m_cond;					/* Idle sleeping on variable */

        CLockedList<CEvent>			m_eventList;			/* Event queue */
        CLockedList<CTimer>			m_timerList;			/* Timer sorted queue */
		CLockedList<CEventReceiver>	m_receiverList;			/* Registered event receivers */

		boolean_t					m_bIterate;				/* Access under m_receiverList lock */
		CEventReceiver*				m_pIterateNext;			/* Access under m_receiverList lock */

		/* Synchronous execution support */
		CCondition					m_condSync;				/* Sync execution awaiting result on variable */
		CSyncBase*					m_pSync;				/* Sync operation in progress */
        
    public:
        CEventLoop(const char* strName);
        virtual ~CEventLoop();

        virtual void sendEvent(CEvent* pEvent);
        virtual void sendEvent(CEvent* pEvent, CEventReceiver* pReceiver);
        void deleteEventAll();

        void insertTimer(CTimer* pTimer);
        void restartTimer(CTimer* pTimer);
        void pauseTimer(CTimer* pTimer);
        boolean_t unlinkTimer(CTimer* pTimer);
        void deleteTimer(CTimer* pTimer);
        void deleteTimerAll();

		virtual void dispatchEvents() {
			processTimers();
			processEvents();
		}

        void registerReceiver(CEventReceiver* pReceiver);
        void unregisterReceiver(CEventReceiver* pReceiver);

        virtual void notify();
        virtual void runEventLoop();
        virtual void stopEventLoop();

		void attachSync(CSyncBase* pSync);
		void detachSync();
		result_t waitSync(hr_time_t hrTimeout);

    protected:
        CTimer* getClosestTimer(hr_time_t hrTime);
        virtual void processTimers();

        CEvent* getNextEvent(hr_time_t hrTime);
        virtual void processEvents();

		hr_time_t getNextIterTime() const;

        /* Optional IDLE handler. WARNING: run under lock */
        virtual void onIdle() {}

    private:
        void cleanup();
        void printTime(hr_time_t hrTime, char* strBuffer, size_t length) const;

#if CARBON_DEBUG_DUMP
    public:
        void dumpTimers(const char* strHead = NULL);
        void dumpReceivers(const char* strHead = NULL);
        void dumpCounts(const char* strPref = "") const;
#endif /* CARBON_DEBUG_DUMP */

#if DEBUG
    protected:
        void checkTimerListEmpty();
        void checkEventListEmpty();
#endif /* DEBUG */
};


/*******************************************************************************
 * Event loop within separate thread
 */

class CEventLoopThread : public CEventLoop
{
    protected:
        CThread         m_thEventLoop;				/* Worker thread */

    public:
        CEventLoopThread(const char* strName);
        virtual ~CEventLoopThread();

    public:
        virtual result_t start();
        virtual void stop();

        boolean_t isRunning() const { return m_thEventLoop.isRunning(); }

    private:
        void* thread(CThread* pThread, void* pData);
};

#endif /* __SHELL_EVENTLOOP_H_INCLUDED__ */
