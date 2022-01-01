/*
 *  Carbon framework
 *  Thread pool
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 10.06.2015 13:22:53
 *      Initial revision.
 */

#ifndef __CARBON_THREAD_POOL_H_INCLUDED__
#define __CARBON_THREAD_POOL_H_INCLUDED__

#include <vector>

#include "shell/atomic.h"

#include "carbon/lock.h"
#include "carbon/event.h"
#include "carbon/event/eventloop.h"

class CThreadPool;

class CThreadPoolItem : public CEventLoopThread, public CEventReceiver
{
	protected:
		CThreadPool*	m_pParent;
		atomic_t		m_busy;

	public:
		CThreadPoolItem(CThreadPool* pParent, const char* strName);
		virtual ~CThreadPoolItem();

	public:
		boolean_t isBusy() const {
			return sh_atomic_get(&m_busy) != 0;
		}

		void setBusy() {
			sh_atomic_set(&m_busy, 1);
		}

	protected:
		virtual void onIdle() {
			sh_atomic_set(&m_busy, 0);
		}

		virtual boolean_t processEvent(CEvent* pEvent) = 0;
};


class CThreadPool : CObject
{
	protected:
		std::vector<CThreadPoolItem*>	m_arThread;
		size_t			m_nMaxThread;
		boolean_t		m_bTerminated;
		mutable CMutex	m_lock;

	public:
		CThreadPool(size_t nMaxThread, const char* strName);
		virtual ~CThreadPool();

	public:
		result_t init(size_t nRunThread = 0);
		void terminate();

		void setMaxThreads(size_t nMaxThread)  {
			m_nMaxThread = nMaxThread;
		}

	protected:
		virtual CThreadPoolItem* createThread(const char* strName) = 0;
		virtual result_t insertThread();

		CThreadPoolItem* get();
		size_t getThreadCount() const;
};


#endif /* __CARBON_THREAD_POOL_H_INCLUDED__ */
