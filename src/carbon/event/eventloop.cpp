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
 *  Revision 1.0, 27.03.2013 12:21:39
 *      Initial revision.
 *
 *  Revision 1.1, 14.02.2017 13:12:12
 *  	Removed std library lists.
 */

#include "carbon/logger.h"
#include "carbon/sync.h"
#include "carbon/event/eventloop.h"


/*******************************************************************************
 * CEventloop class
 */

CEventLoop::CEventLoop(const char* strName) :
    CObject(strName),
	m_bDone(FALSE),
	m_bIterate(FALSE),
	m_pIterateNext(0),
    m_pSync(0)
{
}

CEventLoop::~CEventLoop()
{
    cleanup();
	shell_assert(m_receiverList.getSize() == 0);
#if DEBUG
    checkTimerListEmpty();
    checkEventListEmpty();
#endif /* DEBUG */
}

/*
 * Clear registered event consumers
 */
void CEventLoop::cleanup()
{
    CAutoLock   	locker(m_receiverList);
	CEventReceiver*	pReceiver;

	pReceiver = m_receiverList.getFirst();
    while ( pReceiver != 0 )  {
		m_receiverList.remove(pReceiver);
		pReceiver = m_receiverList.getFirst();
    }

	if ( m_bIterate )  {
		m_pIterateNext = 0;
	}
}    
    
/*
 * Insert a new timer to the event loop processing
 *
 *      pTimer      timer object
 */
void CEventLoop::insertTimer(CTimer* pTimer)
{
    CAutoLock       locker(m_cond);
    hr_time_t       hrTime = pTimer->getTime();
	CTimer*			pCurTimer;
    boolean_t       bInserted = FALSE;

	pCurTimer = m_timerList.getFirst();
    while ( pCurTimer != 0 )  {
        if ( hrTime < pCurTimer->getTime() )  {
            m_timerList.insert_before(pCurTimer, pTimer);
            bInserted = TRUE;
            break;
        }

		pCurTimer = m_timerList.getNext(pCurTimer);
    }

    if ( !bInserted )  {
        m_timerList.insert(pTimer);
    }

    locker.unlock();

    if ( logger_is_enabled(LT_DEBUG|L_EV_TRACE_TIMER) )  {
    	hr_time_t	hrTime1;
    	hr_time_t	hrNow = hr_time_now();
    	char		buffer[64];

    	hrTime1 = pTimer->getTime() > hrNow ? (pTimer->getTime()-hrNow) : HR_0;
    	printTime(hrTime1, buffer, sizeof(buffer));

    	log_dump("---> TM start: %s => %s\n", pTimer->getName(), buffer);
    }

    /* Wakeup an event loop */
    notify();
}

/*
 * Restart timer
 *
 *      pTimer     		timer object
 */
void CEventLoop::restartTimer(CTimer* pTimer)
{
    CAutoLock   locker(m_cond);
	CTimer*		pCurTimer;

	pCurTimer = m_timerList.getFirst();
    while ( pCurTimer != 0 )  {
        if ( pCurTimer == pTimer )  {
            m_timerList.remove(pTimer);
            pTimer->restart();
            locker.unlock();
            insertTimer(pTimer);
            break;
        }

		pCurTimer = m_timerList.getNext(pCurTimer);
    }
}

/*
 * Temporary pause timer
 *
 *      pTimer      timer object
 */
void CEventLoop::pauseTimer(CTimer* pTimer)
{
    CAutoLock   locker(m_cond);
	CTimer*		pCurTimer;

	pCurTimer = m_timerList.getFirst();
    while ( pCurTimer != 0 )  {
        if ( pCurTimer == pTimer )  {
            m_timerList.remove(pTimer);
            pTimer->pause();
            locker.unlock();
            insertTimer(pTimer);
            break;
        }

		pCurTimer = m_timerList.getNext(pCurTimer);
    }
}

/*
 * Remove timer from the timer list
 *
 *      pTimer      timer object
 */
boolean_t CEventLoop::unlinkTimer(CTimer* pTimer)
{
	boolean_t 	bUnlinked = FALSE;

    if ( pTimer )  {
        CAutoLock   locker(m_cond);
		CTimer*		pCurTimer;

		pCurTimer = m_timerList.getFirst();
        while ( pCurTimer != 0 )  {
            if ( pCurTimer == pTimer )  {
                m_timerList.remove(pTimer);
				bUnlinked = TRUE;
                break;
            }

			pCurTimer = m_timerList.getNext(pCurTimer);
        }
    }

	return bUnlinked;
}

/*
 * Delete timer
 *
 *      pTimer      timer object
 */
void CEventLoop::deleteTimer(CTimer* pTimer)
{
    if ( pTimer )  {
        if ( unlinkTimer(pTimer) ) {
			delete pTimer;
		}
    }
}

/*
 * Delete all timers of the event loop
 */
void CEventLoop::deleteTimerAll()
{
    CAutoLock   locker(m_cond);
	CTimer		*pTimer;

	pTimer = m_timerList.getFirst();
    while ( pTimer != 0 )  {
		m_timerList.remove(pTimer);
        delete pTimer;
		pTimer = m_timerList.getFirst();
    }
}

/*
 * Pick up the next expired timer
 *
 *      hrTime			current time
 *
 * Return: timer pointer or NULL
 */
CTimer* CEventLoop::getClosestTimer(hr_time_t hrTime)
{
    CAutoLock   locker(m_cond);
    CTimer*     pClosestTimer = 0, *pTimer;

	pTimer = m_timerList.getFirst();
    while ( pTimer != 0 )  {
        if ( hrTime >= pTimer->getTime() )  {
            m_timerList.remove(pTimer);
            pClosestTimer = pTimer;
            break;
        }

		pTimer = m_timerList.getNext(pTimer);
    }

    return pClosestTimer;
}

/*
 * Execute all expired timers
 */
void CEventLoop::processTimers()
{
    hr_time_t   hrNow = hr_time_now();
    CTimer*     pTimer;

    while ( !m_bDone && (pTimer=getClosestTimer(hrNow)) != 0 )  {
        if ( !pTimer->isPeriodic() )  {
            if ( logger_is_enabled(LT_DEBUG|L_EV_TRACE_TIMER) )  {
            	log_dump("---> TM  exec: %s\n", pTimer->getName());
            }
            pTimer->execute();
            delete pTimer;
        }
        else  {
            if ( logger_is_enabled(LT_DEBUG|L_EV_TRACE_TIMER) )  {
            	log_dump("---> TM  exec: %s\n", pTimer->getName());
            }
            pTimer->restart();
			insertTimer(pTimer);
            pTimer->execute();
        }

        hrNow = hr_time_now();
    }
}

/*
 * Send event to the current event loop for the future processing
 *
 *      pEvent          event object pointer
 */
void CEventLoop::sendEvent(CEvent* pEvent)
{
    CAutoLock   locker(m_cond);

	if ( !m_bDone )  {
		seqnum_t	sessId = pEvent->getSessId();

		if ( sessId != NO_SEQNUM )  {
			m_condSync.lock();
			if ( m_pSync != 0 && m_pSync->getSessId() == sessId )  {
				m_pSync->attachEvent(pEvent);
				pEvent->release();
				m_condSync.unlock();
				m_condSync.wakeup();
				return;
			}
			m_condSync.unlock();
		}

        m_eventList.insert(pEvent);
        locker.unlock();

        if ( logger_is_enabled(LT_DEBUG|L_EV_TRACE_EVENT) )  {
        	char			buffer[64];
        	CEventReceiver*	pReceiver = pEvent->getReceiver();

        	pEvent->dumpEvent(buffer, sizeof(buffer));

        	log_dump("---> EV: %s => %s: %s\n", this->getName(),
                     pReceiver != EVENT_MULTICAST ? pReceiver->getName() :
                     "MULTICAST", buffer);
        }
    
        /* Wakeup an event loop */
        notify();
    }
    else  {
        char	buffer[128];

        pEvent->dumpEvent(buffer, sizeof(buffer));
        log_debug(L_GEN, "[eventloop(%s)] event loop is shutting down, event has been dropped (%s)\n",
                                getName(), buffer);
        pEvent->release();
    }        
}

/*
 * Send event to the current event loop for the future processing
 *
 *      pEvent          event object pointer
 *      pReceiver       receiver object pointer
 */
void CEventLoop::sendEvent(CEvent* pEvent, CEventReceiver* pReceiver)
{
    pEvent->setReceiver(pReceiver);
    sendEvent(pEvent);
}

/*
 * Pick up a next event for the processing
 *
 *      hrTime      process events sent before
 *
 * Return: event pointer or 0
 */
CEvent* CEventLoop::getNextEvent(hr_time_t hrTime)
{
    CAutoLock   locker(m_cond);
    CEvent*     pNextEvent = 0, *pEvent;

	pEvent = m_eventList.getFirst();
    while ( pEvent != 0 )  {
        if ( hrTime > pEvent->getTime() )  {
            m_eventList.remove(pEvent);
            pNextEvent = pEvent;
            break;
        }

        pEvent = m_eventList.getNext(pEvent);
    }

    return pNextEvent;
}

/*
 * Process all events for the current event loop
 */
void CEventLoop::processEvents()
{
    CEvent*     pEvent;
    hr_time_t   hrProcessTime = hr_time_now();

    while ( !m_bDone && (pEvent = getNextEvent(hrProcessTime)) != NULL )  {
        CAutoLock   			locker(m_receiverList);
        const CEventReceiver* 	pEventReceiver = pEvent->getReceiver();
		CEventReceiver*			pReceiver;

		shell_assert(!m_bIterate);
		m_bIterate = TRUE;
		m_pIterateNext = m_receiverList.getFirst();

        while ( !m_bDone && m_pIterateNext != 0 )  {
			pReceiver = m_pIterateNext;
			m_pIterateNext = m_receiverList.getNext(pReceiver);

            if ( pReceiver == pEventReceiver || pEventReceiver == EVENT_MULTICAST )  {
				locker.unlock();
                pReceiver->processEvent(pEvent);
				locker.lock();
            }
        }

		m_bIterate = FALSE;

        pEvent->release();
    }
}

/*
 * Remove all events of the event loop
 */
void CEventLoop::deleteEventAll()
{
    CAutoLock   locker(m_cond);
    CEvent		*pEvent, *pTmpEvent;

	pEvent = m_eventList.getFirst();
    while ( pEvent != 0 )  {
		pTmpEvent = pEvent;
		pEvent = m_eventList.getNext(pEvent);

		m_eventList.remove(pTmpEvent);
        pTmpEvent->release();
    }
}

/*
 * Register event destination object at the event loop
 */
void CEventLoop::registerReceiver(CEventReceiver* pReceiver)
{
    CAutoLock   locker(m_receiverList);

	//log_debug(L_GEN, "[eventloop(%s)] register event consumer: %p\n", getName(), pReceiver);

    shell_assert(pReceiver != 0);
    m_receiverList.insert(pReceiver);
}

/*
 * Unregister event destination object at the event loop
 */
void CEventLoop::unregisterReceiver(CEventReceiver* pReceiver)
{
    CAutoLock   locker(m_receiverList);
	boolean_t	bFound;

	//log_debug(L_GEN, "[eventloop(%s)] unregister event consumer: %p\n", getName(), pReceiver);

    shell_assert(pReceiver != 0);
    
    bFound = m_receiverList.find(pReceiver);
    if ( bFound )  {
		if ( m_bIterate && m_pIterateNext == pReceiver )  {
			m_pIterateNext = m_receiverList.getNext(pReceiver);
		}
		m_receiverList.remove(pReceiver);
    }
    else  {
    	log_debug(L_GEN, "[eventloop(%s)] receiver %s (%Xh) not found\n",
				  getName(), pReceiver->getName(), pReceiver);
    }        
}

/*
 * Calculate sleep time for the event loop
 *
 * Return: timeout to sleep
 */
hr_time_t CEventLoop::getNextIterTime() const
{
    CTimer*     pTimer;
    hr_time_t   hrNextIterTime;

    if ( !m_timerList.isEmpty() )  {
        pTimer = m_timerList.getFirst();
        hrNextIterTime = pTimer->getTime();
    }
    else  {
        hrNextIterTime = hr_time_now() + EVENT_LOOP_ITERATION_TIMEOUT;
    }

    if ( hr_time_now() >= hrNextIterTime || m_eventList.getSize() != 0 ) {
        hrNextIterTime = HR_0;
    }

    return hrNextIterTime;
}

/*
 * The main processing loop of the event loop object
 */
void CEventLoop::runEventLoop()
{
    hr_time_t   hrNextIterTime;

    while ( !m_bDone )  {
		dispatchEvents();
        m_cond.lock();

        if ( getNextIterTime() != HR_0 && !m_bDone ) {
            onIdle();
        }
        if ( (hrNextIterTime=getNextIterTime()) != 0 && !m_bDone ) {
            m_cond.waitTimed(hrNextIterTime);
        }

        m_cond.unlock();
    }

    //deleteTimerAll();
    //deleteEventAll();
}

/*
 * Terminating the event loop
 */
void CEventLoop::stopEventLoop()
{
    m_cond.lock();
    m_bDone = TRUE;
    m_cond.wakeup();
    m_cond.unlock();
}

/*
 * Notify the event loop of any kind of external events (events, timers, ...)
 */
void CEventLoop::notify()
{
    m_cond.lock();
    m_cond.wakeup();
    m_cond.unlock();
}

void CEventLoop::attachSync(CSyncBase* pSync)
{
	m_condSync.lock();

	shell_assert(m_pSync == 0);
	if ( m_pSync != 0 )  {
		log_error(L_GEN, "[eventloop(%s)] m_pSync is not 0 !!!\n", getName());
	}

	m_pSync = pSync;
	/* log_debug(L_GEN, "[eventloop(%s)] ATTACHED Sync Object %p\n", getName(), pSync); */

	/* Leave sync locked ! */
}

void CEventLoop::detachSync()
{
	/* The sync lock must be held */
	if ( m_pSync == 0 )  {
		log_error(L_GEN, "[eventloop(%s)] sync has gone away\n", getName());
	}

	/* log_debug(L_GEN, "[eventloop(%s)] DETTACHED Sync Object %p\n", getName(), m_pSync); */

	m_pSync = 0;
	m_condSync.unlock();
}

/*
 * Waiting to execute sync function
 *
 * 		hrTimeout			maximum time to wait
 *
 * Return: ESUCCESS, ETIMEDOUT, EFAULT, EBADE
 *
 * Note: Sync lock must be held
 */
result_t CEventLoop::waitSync(hr_time_t hrTimeout)
{
    result_t    nresult;

	if ( m_bDone )  {
		/* Event loop is shutting down */
		log_debug(L_GEN, "[eventloop(%s)] eventloop is shutting down\n", getName());
		return ETIMEDOUT;
	}

	shell_assert(m_pSync);
    if ( !m_pSync )  {
        log_error(L_GEN, "[eventloop(%s)] sync is not attached\n", getName());
        return EFAULT;
    }

	m_condSync.waitTimed(hr_time_now()+hrTimeout);

    if ( !m_pSync )  {
        log_error(L_GEN, "[eventloop(%s)] sync has gone away\n", getName());
        return EBADE;
    }

    nresult = m_pSync->processSyncEvent();
    return nresult;
}

/*
 * Convert time to the string
 *
 *      hrTime          time to convert
 *      strBuffer       string buffer
 *      length          string buffer size
 */
void CEventLoop::printTime(hr_time_t hrTime, char* strBuffer, size_t length) const
{
	hr_time_t	hrTemp, hr;
	size_t		l = 0;

	hrTemp = hrTime;
	hr = hrTemp/HR_1DAY;
	if ( hr > 0 ) {
		l = (size_t)_tsnprintf(strBuffer, length, "%d day(s) ", (int)hr);
		hrTemp = hrTemp%HR_1DAY;
	}

	hr = hrTemp/HR_1HOUR;
	if ( (hr > 0 || l != 0) && l < length ) {
		l += _tsnprintf(&strBuffer[l], length-l, "%d hour(s) ", (int)hr);
		hrTemp = hrTemp%HR_1HOUR;
	}

	hr = hrTemp/HR_1MIN;
	if ( (hr > 0 || l != 0) && l < length ) {
		l += _tsnprintf(&strBuffer[l], length-1, "%d min(s) ", (int)hr);
		hrTemp = hrTemp%HR_1MIN;
	}

	if ( l < length )  {
		l += _tsnprintf(&strBuffer[l], length-1, "%d.%03d sec(s)",
				(int)(hrTemp/HR_1SEC), (int)(hrTemp%HR_1SEC));
	}
}

#if DEBUG

void CEventLoop::checkTimerListEmpty()
{
    CAutoLock   locker(m_cond);
    int			n = m_timerList.getSize();

    if ( n > 0 )  {
		log_dump("*** TimerList is not empty: (count=%d) ***\n", n);
#if CARBON_DEBUG_DUMP
		dumpTimers();
#endif /* CARBON_DEBUG_DUMP */
    }

    shell_assert(n == 0);
}

void CEventLoop::checkEventListEmpty()
{
	CAutoLock   locker(m_cond);
	CEvent*		pEvent;
    int			n = 0;
    char		buffer[128];

	pEvent = m_eventList.getFirst();
    while ( pEvent != 0 )  {
    	if ( n++ == 0 )  {
    		log_dump("*** EventList is not empty: ***\n");
    	}

    	pEvent->dumpEvent(buffer, sizeof(buffer));
    	log_dump("--- Event: %s\n", buffer);
        pEvent = m_eventList.getNext(pEvent);
    }

    /*if ( n != 0 )  {
    	shell_assert(FALSE);
    }*/
}

#endif /* DEBUG */

#if CARBON_DEBUG_DUMP

void CEventLoop::dumpTimers(const char* strPref)
{
	CTimer*		pTimer;

	if ( strPref )  {
		log_dump("*** Timer list (%d) ***\n", m_timerList.getSize());
	}

	pTimer = m_timerList.getFirst();
	while ( pTimer != 0 )  {
		log_dump("--- Timer: %s\n", pTimer->getName());
		pTimer = m_timerList.getNext(pTimer);
	}
}

void CEventLoop::dumpReceivers(const char* strPref)
{
	CEventReceiver*		pReceiver;

	if ( strPref )  {
		log_dump("*** Event receiver list (%d): %s ***\n",
				 m_receiverList.getSize(), strPref);
	}

	pReceiver = m_receiverList.getFirst();
	while ( pReceiver != 0 )  {
		log_dump("--- Receiver: %Xh\n", A(pReceiver));
		pReceiver = m_receiverList.getNext(pReceiver);
	}
}

void CEventLoop::dumpCounts(const char* strPref) const
{
	log_dump("%s: Receivers: list=%d\n", strPref, m_receiverList.getSize());
}

#endif /* CARBON_DEBUG_DUMP */


/*******************************************************************************
 * CEventLoopThread
 */

CEventLoopThread::CEventLoopThread(const char* strName) :
    CEventLoop(strName),
    m_thEventLoop(strName)
{
}

CEventLoopThread::~CEventLoopThread()
{
    shell_assert(!m_thEventLoop.isRunning());
}

/*
 * Eventloop thread function
 *
 *      pThread     eventloop's thread object pointer
 *      pData       thread custom data (unused so far)
 */
void* CEventLoopThread::thread(CThread* pThread, void* pData)
{
	UNUSED(pData);

    pThread->bootCompleted(ESUCCESS);

    log_debug(L_GEN_FL, "[eventloop_th(%s)] starting eventloop\n", getName());
    runEventLoop();
    log_debug(L_GEN_FL, "[eventloop_th(%s)] stopped eventloop\n", getName());

    return NULL;
}

/*
 * Start event loop
 *
 * Return: ESUCCESS, ...
 */
result_t CEventLoopThread::start()
{
    result_t	nresult;

    nresult = m_thEventLoop.start(THREAD_CALLBACK(CEventLoopThread::thread, this));
    if ( nresult != ESUCCESS )  {
        log_error(L_GEN, "[eventloop_th(%s)] failed to start thread, result: %d\n",
                  getName(), nresult);
    }

    return nresult;
}

/*
 * Stop eventloop and thread processing
 */
void CEventLoopThread::stop()
{
	stopEventLoop();

	m_condSync.lock();
	if ( m_pSync != 0 ) {
		m_condSync.wakeup();
		m_pSync = 0;
	}
	m_condSync.unlock();

	m_thEventLoop.stop();

	//deleteEventAll();
	//deleteTimerAll();

	m_bDone = FALSE;
}
