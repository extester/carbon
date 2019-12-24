/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP Video post processor base class
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 24.10.2016 10:26:15
 *		Initial revision.
 */

#include "net_media/rtp_playout_buffer.h"
#include "net_media/media_sink.h"

#define VIDEO_SINKV_IDLE			HR_8SEC

/*******************************************************************************
 * CVideoSink class
 */

CVideoSink::CVideoSink() :
	m_nFps(0),
	m_nRate(0)
{
}

CVideoSink::~CVideoSink()
{
}


/*******************************************************************************
 * CVideoSinkV class
 */

CVideoSinkV::CVideoSinkV() :
	CVideoSink(),
	CThread("video_sink", HR_0, HR_4SEC),
	m_nNodes(ZERO_COUNTER)
{
	atomic_set(&m_bDone, FALSE);
}

CVideoSinkV::~CVideoSinkV()
{
}

/*
 * Wake up a sink thread to process outstanding completed nodes
 */
void CVideoSinkV::wakeup()
{
	m_cond.lock();
	m_cond.wakeup();
	m_cond.unlock();
}

/*
 * Remove all awaiting nodes from the list
 */
void CVideoSinkV::clear()
{
	CAutoLock			locker(m_cond);
	CRtpPlayoutNode*	pNode = 0;

	while ( !m_arNode.empty() ) {
		pNode = *m_arNode.begin();
		m_arNode.pop_front();
		pNode->release();
	}
}

/*
 * Put new completed node to the list and wakeup sink thread
 */
void CVideoSinkV::put(CRtpPlayoutNode* pNode)
{
	CAutoLock			locker(m_cond);

	if ( atomic_get(&m_bDone) == FALSE ) {
		pNode->reference();
		m_arNode.push_back(pNode);
		counter_inc(m_nNodes);
		m_cond.wakeup();
	}
}

/*
 * Pick up next available complete node for processing
 *
 * Return: node pointer or 0 if no complete nodes
 */
CRtpPlayoutNode* CVideoSinkV::getNextNode()
{
	CAutoLock			locker(m_cond);
	CRtpPlayoutNode*	pNode = 0;

	if ( !m_arNode.empty() )  {
		pNode = *m_arNode.begin();
		m_arNode.pop_front();
	}

	return pNode;
}

/*
 * Sink worker thread
 */
void* CVideoSinkV::threadProc(CThread* pThread, void* pData)
{
	CRtpPlayoutNode*	pNode;

	while ( atomic_get(&m_bDone) == FALSE )  {
		pNode = getNextNode();
		if ( pNode )  {
			processNode(pNode);
			pNode->release();
		}

		m_cond.lock();
		if ( atomic_get(&m_bDone) == FALSE && m_arNode.size() == 0 )  {
			m_cond.waitTimed(hr_time_now()+VIDEO_SINKV_IDLE);
		}
		m_cond.unlock();
	}

	return NULL;
}

/*
 * Initialise and start sink processing
 *
 * Return: ESUCCESS, ....
 */
result_t CVideoSinkV::init(int nFps, int nRate)
{
	result_t	nresult;

	nresult = CVideoSink::init(nFps, nRate);
	if ( nresult != ESUCCESS ) {
		return nresult;
	}

	atomic_set(&m_bDone, FALSE);

	nresult = CThread::start(THREAD_CALLBACK(CVideoSinkV::threadProc, this), 0);
	if ( nresult != ESUCCESS )  {
		log_error(L_RTP, "[sinkv] failed to start thread, result: %d\n", nresult);
		CVideoSink::terminate();
	}

	return nresult;
}

/*
 * Stop sink
 */
void CVideoSinkV::terminate()
{
	atomic_set(&m_bDone, TRUE);
	wakeup();
	CThread::stop();
	clear();
}

/*******************************************************************************
 * Debugging support
 */

void CVideoSinkV::dump(const char* strPref) const
{
	CAutoLock	locker(m_cond);

	log_dump("*** %sVideoSinkV: pending nodes: %u, full node count: %u\n",
			 strPref, m_arNode.size(), counter_get(m_nNodes));
}
