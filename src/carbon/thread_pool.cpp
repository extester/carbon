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
 *  Revision 1.0, 10.06.2015 15:07:43
 *      Initial revision.
 */

#include "shell/error.h"

#include "carbon/logger.h"
#include "carbon/thread_pool.h"


/*******************************************************************************
 * CThreadPoolItem base virtual class
 */

CThreadPoolItem::CThreadPoolItem(CThreadPool* pParent, const char* strName) :
	CEventLoopThread(strName),
	CEventReceiver(this, strName),
	m_pParent(pParent)
{
	sh_atomic_set(&m_busy, 0);
}

CThreadPoolItem::~CThreadPoolItem()
{
}


/*******************************************************************************
 * CThreadPool base virtual class
 */

CThreadPool::CThreadPool(size_t nMaxThread, const char* strName) :
	CObject(strName),
	m_nMaxThread(nMaxThread),
	m_bTerminated(FALSE)
{
}

CThreadPool::~CThreadPool()
{
}

/*
 * Get thread count in the pool
 *
 * Return: thread count
 */
size_t CThreadPool::getThreadCount() const
{
	CAutoLock	locker(m_lock);

	return m_arThread.size();
}

/*
 * Add new thread to the pool
 *
 * Return: ESUCCESS, ERANGE - pool overflow, ...
 *
 * Note: pool must be locked.
 */
result_t CThreadPool::insertThread()
{
	CThreadPoolItem*	item;
	char				strTmp[64];
	result_t			nresult;

	if ( m_arThread.size() >= m_nMaxThread )  {
		log_error(L_GEN, "[thread_pool] %s: too many thread created\n", getName());
		return ERANGE;
	}

	_tsnprintf(strTmp, sizeof(strTmp), "%s:pool_thread_%u", getName(), (unsigned)m_arThread.size());
	item = createThread(strTmp);
	nresult = item->start();
	if ( nresult == ESUCCESS )  {
		m_arThread.push_back(item);
	}
	else {
		delete item;
	}

	return nresult;
}

/*
 * Get available thread from the pool
 *
 * Return: thread pool item pointer or NULL
 */
CThreadPoolItem* CThreadPool::get()
{
	CAutoLock			locker(m_lock);
	CThreadPoolItem*	pItem = 0;
	size_t				i, count = m_arThread.size();
	static size_t		nLRU = 0;

	if ( m_bTerminated )  {
		return 0;
	}

	/*
	 * Find an IDLE thread
	 */
	for(i=0; i<count; i++)  {
		if ( !m_arThread[i]->isBusy() )  {
			pItem = m_arThread[i];
			pItem->setBusy();
			break;
		}
	}

	if ( pItem != 0 )  {
		return pItem;
	}

	/*
	 * Use extra thread
	 */
	if ( count < m_nMaxThread )  {
		result_t	nresult;

		nresult = insertThread();
		if ( nresult == ESUCCESS )  {
			pItem = m_arThread.back();
			pItem->setBusy();
			return pItem;
		}

		log_error(L_GEN, "[thread_pool] %s: failed to run extra thread, result: %d\n",
				  getName(), nresult);
	}

	/*
	 * Run busy thread in LRU order
	 */
	if ( nLRU > m_arThread.size() )  {
		nLRU = 0;
	}

	if ( nLRU < m_arThread.size() )  {
		nLRU++;
		m_arThread[nLRU-1]->setBusy();
		return m_arThread[nLRU-1];
	}

	log_error(L_GEN, "[thread_pool] %s: can't get any thread!!!\n", getName());
	return 0;
}

/*
 * Initialise thread pool and run pre-running thread
 *
 * Return: ESUCCESS, ...
 */
result_t CThreadPool::init(size_t nRunThread)
{
	CAutoLock	locker(m_lock);
	size_t 		i;

	shell_assert(nRunThread <= m_nMaxThread);

	m_bTerminated = FALSE;
	m_arThread.reserve(m_nMaxThread);

	for(i=0; i<nRunThread; i++)  {
		insertThread();
	}

	if ( m_arThread.size() != nRunThread )  {
		log_warning(L_GEN, "[thread_pool] %s: pre-running %d of %d thread(s)\n",
						getName(), m_arThread.size(), nRunThread);
	}

	return ESUCCESS;
}

/*
 * Terminate and delete all threads in the pool
 * and release pool resources
 */
void CThreadPool::terminate()
{
	CAutoLock			locker(m_lock);
	CThreadPoolItem*	pItem;

	m_bTerminated = TRUE;
	while ( m_arThread.size() > 0 )  {
		pItem = m_arThread.back();
		m_arThread.pop_back();

		locker.unlock();
		pItem->stop();
		delete pItem;
		locker.lock();
	}
}
