/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP Playout Buffer
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 14.10.2016 17:42:00
 *		Initial revision.
 */

#include "net_media/rtp_frame_cache.h"
#include "net_media/rtp_input_queue.h"
#include "net_media/media_sink.h"
#include "net_media/rtp_session.h"
#include "net_media/rtp_playout_buffer.h"

/*
 * Compare a < b, unsigned 32 with overflow
 */
//#define compareLt32(__a, __b)	( (((__a) - (__b)) & 0x80000000) != 0 )

/*******************************************************************************
 * CRtpPlayoutNode class
 */

CRtpPlayoutNode::CRtpPlayoutNode(uint64_t rtpRealTimestamp, hr_time_t hrPlayoutTime,
								 CRtpPlayoutBuffer* pParent) :
	CMediaFrame(pParent->getName()),
	m_pParent(pParent),
	m_nLength(0),
	m_rtpRealTimestamp(rtpRealTimestamp),
	m_hrCreationTime(hr_time_now()),
	m_hrPlayoutTime(hrPlayoutTime),
	m_hrPts(HR_0),
	m_nCurDelay(0)
{
	queue_init(&m_queue);
}

CRtpPlayoutNode::~CRtpPlayoutNode()
{
	clear();
}

/*
 * Remove all frame out of node
 */
void CRtpPlayoutNode::clear()
{
	rtp_frame_t*	pFrame;

	while ( !queue_empty(&m_queue) )  {
		queue_remove_first(&m_queue, pFrame, rtp_frame_t*, link);
		pFrame->pOwner->put(pFrame);
	}

	m_nLength = 0;
}

/*
 * Insert frame to the linked list in sequence number order
 *
 * 		pFrame		frame to insert
 *
 * Return:
 * 		ESUCCESS		frame inserted
 * 		EINVAL			frame is duplicated or node is completed already
 */
result_t CRtpPlayoutNode::insertFrame(rtp_frame_t* pFrame)
{
	uint64_t		frameSeqNum, seqNum;
	rtp_frame_t*	elt;
	boolean_t		bInserted = FALSE, bDuplicated = FALSE;

	/* TODO: CHECK TIMESTAMP IN VALID INTERVAL */

	frameSeqNum = pFrame->nSeqNum;
	queue_iterate_back(&m_queue, elt, rtp_frame_t*, link) {
		seqNum = RTP_HEAD_SEQUENCE(elt->head.fields);

		if ( frameSeqNum == seqNum )  {
			/* Dumplicated frame */
			bDuplicated = TRUE;
			//log_error(L_RTP, "[rtp_playout_node(%s)] -- DUPLICATED FRAME (seqnum %llu) --\n",
			//		  	getName(), seqNum);
			break;
		}

		if ( frameSeqNum > seqNum )  {
			queue_insert_after(&m_queue, pFrame, elt, rtp_frame_t*, link);
			m_nLength++;
			bInserted = TRUE;
			break;
		}
	}

	if ( !bInserted && !bDuplicated )  {
		queue_enter_first(&m_queue, pFrame, rtp_frame_t*, link);
		m_nLength++;
		bInserted = TRUE;
	}

	return bInserted ? ESUCCESS : EINVAL;
}

/*******************************************************************************
 * Debugging support
 */

/*
 * Check for RTP frame sequence number orders
 *
 * Return: count of invalid orders
 */
int CRtpPlayoutNode::validateSeqOrder() const
{
	int					nInvalid = 0;
	const rtp_frame_t*	elt;
	uint16_t			nextSeq, seq;

	if ( m_nLength > 0 ) {
		elt = (const rtp_frame_t*)queue_first(&m_queue);
		nextSeq = RTP_HEAD_SEQUENCE(elt->head.fields);

		queue_iterate(&m_queue, elt, const rtp_frame_t*, link) {
			seq = RTP_HEAD_SEQUENCE(elt->head.fields);

			if ( seq != nextSeq ) {
				nInvalid++;
			}
			nextSeq = seq+1;
		}
	}

	return nInvalid;
}

/*
 * Check for equal timestamp of the all RTP frames
 *
 * Return: wrong timestamp RTP frame count
 */
int CRtpPlayoutNode::validateTimestamp() const
{
	int					nInvalid = 0;
	const rtp_frame_t*	elt;
	uint32_t			timestamp;

	if ( m_nLength > 0 ) {
		elt = (const rtp_frame_t*)queue_first(&m_queue);
		timestamp = elt->head.timestamp;

		queue_iterate(&m_queue, elt, const rtp_frame_t*, link) {
			if ( elt->head.timestamp != timestamp )  {
				nInvalid++;
			}
		}
	}

	return nInvalid;
}

/*******************************************************************************
 * Debugging support
 */

void CRtpPlayoutNode::dump(const char* strPref) const
{
	int 				nInvalidOrder, nInvalidTimestamp;
	const rtp_frame_t*	elt;
	uint32_t			timestamp = 0;
	hr_time_t			hrFullAriveTime = 0;

	if ( m_nLength > 0 )  {
		hr_time_t	hrMin = HR_FOREVER, hrMax = HR_0, hrPrev = HR_0;

		elt = (const rtp_frame_t*)queue_first(&m_queue);
		timestamp = elt->head.timestamp;

		queue_iterate(&m_queue, elt, const rtp_frame_t*, link) {
			if ( hrPrev != HR_0 )  {
				if ( hrMin > elt->hrArriveTime )  {
					hrMin = elt->hrArriveTime;
				}
				if ( hrMax < elt->hrArriveTime )  {
					hrMax = elt->hrArriveTime;
				}
			}
			hrPrev = elt->hrArriveTime;
		}

		hrFullAriveTime = hrMax - hrMin;
	}

	log_dump("%s>> Node timestamp %u, frames: %d, playout time: %llu, Receive time: %u ms\n",
			 strPref, timestamp, m_nLength, m_hrPlayoutTime,
			 HR_TIME_TO_MILLISECONDS(hrFullAriveTime));


	nInvalidOrder = validateSeqOrder();
	if ( nInvalidOrder != 0 )  {
		log_dump("%s************************************************\n", strPref);
		log_dump("%s*** Detected invalid frame sequence order %d ***\n", strPref, nInvalidOrder);
		log_dump("%s************************************************\n", strPref);
	}

	nInvalidTimestamp = validateTimestamp();
	if ( nInvalidTimestamp != 0 )  {
		log_dump("%s**************************************\n", strPref);
		log_dump("%s*** Detected invalid timestamps %d ***\n", strPref, nInvalidOrder);
		log_dump("%s**************************************\n", strPref);
	}
}


/*******************************************************************************
 * CRptPlayoutBuffer class
 */

CRtpPlayoutBuffer::CRtpPlayoutBuffer(int nProfile, int nFps, int nClockRate,
						int nMaxInputQueue, int nMaxDelay, const char* strName) :
	CThread(strName, HR_0, HR_8SEC),
	m_pSink(0),
	m_nProfile(nProfile),
	m_nFps(nFps),
	m_nClockRate(nClockRate),
	m_rtpSourceInited(FALSE),
	m_lastPlayoutTimestamp(0),
	m_pSessionManager(0),

	m_nMaxDelay(nMaxDelay),

	m_nFrameCount(ZERO_COUNTER),
	m_nFrameDropped(ZERO_COUNTER),
	m_nFrameLate(ZERO_COUNTER),
	m_nNodeDropped(ZERO_COUNTER),
	m_nNodeMaxCount(ZERO_COUNTER)
{
	m_pInputQueue = new CRtpInputQueue(nMaxInputQueue);
	atomic_set(&m_bDone, FALSE);

	shell_assert_ex(nMaxDelay >= 0 && nMaxDelay <= (m_nFps*2), "nMaxDelay: %d", nMaxDelay);
}

CRtpPlayoutBuffer::~CRtpPlayoutBuffer()
{
	shell_assert(m_pInputQueue->getLength() == 0);
	shell_assert(m_arNode.size() == 0);
	SAFE_DELETE(m_pInputQueue);
}

void CRtpPlayoutBuffer::clear()
{
	CRtpPlayoutNode*	pNode;

	if ( m_pInputQueue )  {
		m_pInputQueue->clear();
	}

	CAutoLock	locker(m_cond);
	while ( !m_arNode.empty() ) {
		pNode = *m_arNode.begin();
		m_arNode.pop_front();
		pNode->release();
	}
}

void CRtpPlayoutBuffer::putFrame(rtp_frame_t* pFrame)
{
	m_pInputQueue->put(pFrame);
	wakeup();
}

void CRtpPlayoutBuffer::wakeup()
{
	m_cond.lock();
	m_cond.wakeup();
	m_cond.unlock();
}

hr_time_t CRtpPlayoutBuffer::getNextWakeupTime() const
{
	std::list<CRtpPlayoutNode*>::const_iterator		it;
	hr_time_t	hrNextTime = hr_time_now()+HR_8SEC;

	for(it=m_arNode.begin(); it != m_arNode.end(); it++) {
		if ( (*it)->getPlayoutTime() < hrNextTime ) {
			hrNextTime = (*it)->getPlayoutTime();
		}
	}

	return hrNextTime;
}

/*
 * Find a node with specified local RTP timestamp
 *
 * 		hrTimestamp		local RTP timestamp of the node to find
 *
 * Return: node pointer or 0
 */
CRtpPlayoutNode* CRtpPlayoutBuffer::findNode(uint64_t rtpRealTimestamp) const
{
	CRtpPlayoutNode*	pNode = 0;
	std::list<CRtpPlayoutNode*>::const_iterator		it;

	for(it=m_arNode.begin(); it != m_arNode.end(); it++)  {
		if ( (*it)->getTimestamp() == rtpRealTimestamp )  {
			pNode = *it;
			break;
		}
	}

	return pNode;
}

/*
 * Insert new node to the list in timestamp order
 *
 * 		pNode		new created node
 */
void CRtpPlayoutBuffer::insertNode(CRtpPlayoutNode* pNode)
{
	std::list<CRtpPlayoutNode*>::reverse_iterator	it;
	uint64_t	nodeRtpTimestamp = pNode->getTimestamp();
	boolean_t	bInserted = FALSE;

	for(it=m_arNode.rbegin(); it != m_arNode.rend(); it++) {
		if ( nodeRtpTimestamp > (*it)->getTimestamp() )  {
			std::list<CRtpPlayoutNode*>::iterator 	itn = it.base();
			m_arNode.insert(itn, pNode);
			bInserted = TRUE;
			break;
		}
	}

	if ( !bInserted )  {
		m_arNode.push_back(pNode);
	}

	if ( (int)m_arNode.size() > counter_get(m_nNodeMaxCount) ) {
		counter_inc(m_nNodeMaxCount);
	}
}

#define RTP_MAP_TO_LOCAL_TIMELINE(__rtpTimestamp)	\
	((hr_time_t)(HR_1SEC*(__rtpTimestamp)/m_nClockRate))

/*
 * Get all frames from the input queue and link them to the
 * appropiate nodes in sequence number order
 */
void CRtpPlayoutBuffer::getInputFrames()
{
	rtp_frame_t*		pFrame;
	CRtpPlayoutNode*	pNode;

	uint16_t			seqNum;
	uint64_t			rtpRealTimestamp;

	hr_time_t			diff;
	boolean_t			bSeqValid;

	result_t			nresult;

	while ( (pFrame=m_pInputQueue->get()) != RTP_FRAME_NULL )  {
		counter_inc(m_nFrameCount);

		//dumpRtpFrame(pFrame, RTP_FRAME_DUMP_HEAD | RTP_FRAME_DUMP_PAYLOAD);

		if ( !m_rtpSourceInited )  {
			/* Preinit source sequence number parameters */
			rtp_boot_source(&m_rtpSource, RTP_HEAD_SEQUENCE(pFrame->head.fields));
			m_rtpSource.hrTimelineBase = hr_time_now();
			m_rtpSource.time_cycles = 0;//RTP_TIMELINE_MOD; !!!!!!! XXX TTT
			m_rtpSource.max_time = pFrame->head.timestamp;
			m_rtpSource.hrTimeOffset = m_rtpSource.hrTimelineBase -
							RTP_MAP_TO_LOCAL_TIMELINE(pFrame->head.timestamp);

			m_rtpSourceInited = TRUE;
		}

		/* Check for the valid frame sequence number */
		seqNum = RTP_HEAD_SEQUENCE(pFrame->head.fields);
		bSeqValid = rtp_update_seq(&m_rtpSource, seqNum);
		if ( !bSeqValid )  {
			pFrame->pOwner->put(pFrame);
			counter_inc(m_nFrameDropped);
			continue;
		}
/****/
		uint32_t 	rtpTimestamp = pFrame->head.timestamp;
		uint32_t 	udelta = rtpTimestamp - m_rtpSource.max_time;

		if ( udelta < MAX_DROPOUT_TIMELINE )  {
			if ( rtpTimestamp < m_rtpSource.max_time ) {
				m_rtpSource.time_cycles += RTP_TIMELINE_MOD;
				log_debug(L_RTP, "[rtp_playout(%s)] *************** RTP timestamp overflow!!!\n", getName());
			}
			m_rtpSource.max_time = rtpTimestamp;
		}
/****/

		/* XXX */
		shell_assert(m_pSessionManager);
		m_pSessionManager->onRtpFrame(pFrame, &m_rtpSource, m_nClockRate);

		/* Calculate a real sequence number */
		pFrame->nSeqNum = m_rtpSource.cycles + seqNum;
		//if ( seqNum > m_rtpSource.max_seq )  {
		//	pFrame->nSeqNum -= RTP_SEQ_MOD;
		//}

		/*
		 * Calculate a real RTP timestamp
		 */
		rtpRealTimestamp = m_rtpSource.time_cycles + pFrame->head.timestamp;
		//if ( pFrame->head.timestamp > m_rtpSource.max_time )  {
			//rtpRealTimestamp -= RTP_TIMELINE_MOD;
		//}

		diff = pFrame->hrArriveTime - RTP_MAP_TO_LOCAL_TIMELINE(rtpRealTimestamp);
		if ( m_rtpSource.hrTimeOffset > diff )  {
			m_rtpSource.hrTimeOffset = diff;
		}

		//log_debug(L_GEN, "[rtp_playout] Real Seqnum: %u => %u, Real Timestamp %u => %llu\n",
		//		  	seqNum, pFrame->nSeqNum, pFrame->head.timestamp, rtpRealTimestamp);

		if ( rtpRealTimestamp <= m_lastPlayoutTimestamp )  {
			/* Frame too late, node is played/dropped */
			pFrame->pOwner->put(pFrame);
			counter_inc(m_nFrameLate);
			log_error(L_RTP, "[rpt_playout(%s)] --- FRAME TOO LATE, DROPPED ---\n", getName());
			continue;
		}

		/*
		 * Append frame to the node
		 */

		nresult = ESUCCESS;
		pNode = findNode(rtpRealTimestamp);
		if ( pNode ) {
			nresult = pNode->insertFrame(pFrame);
		}
		else {
			pNode = createNode(pFrame, rtpRealTimestamp);
			if ( pNode )  {
				insertNode(pNode);
			}
			else {
				nresult = EINVAL;
			}
#if 0
			/* Update timeline parameters */
			uint32_t 	rtpTimestamp = pFrame->head.timestamp;
			uint32_t 	udelta = rtpTimestamp - m_rtpSource.max_time;

			if ( udelta < MAX_DROPOUT )  {
				if ( rtpTimestamp < m_rtpSource.max_time ) {
					m_rtpSource.time_cycles += RTP_TIMELINE_MOD;
					log_debug(L_RTP, "[rtp_playout] *************** RTP timestap overflow!!!\n");
				}
				m_rtpSource.max_time = rtpTimestamp;
			}
#endif
		}

		if ( nresult != ESUCCESS )  {
			pFrame->pOwner->put(pFrame);
			counter_inc(m_nFrameDropped);
			log_debug(L_RTP, "[rtp_playout(%s)] duplicated/invalid frame!\n", getName());
		}
	}
}

void CRtpPlayoutBuffer::playout()
{
	std::list<CRtpPlayoutNode*>::iterator	it;
	CRtpPlayoutNode*	pNode;
	hr_time_t			hrNow = hr_time_now();
	int 				nDelayCount;

	while ( (it=m_arNode.begin()) != m_arNode.end() )  {
		if ( (*it)->getPlayoutTime() > hrNow )  {
			break;
		}

		pNode = *it;
		if ( pNode->isReady() )  {
			m_lastPlayoutTimestamp = pNode->getTimestamp();
			it = m_arNode.erase(it);
			m_pSink->put(pNode);
			pNode->release();
			continue;
		}

		/* Node delay */
		nDelayCount = pNode->incrDelayCount();
		if ( nDelayCount < m_nMaxDelay )  {
			//log_debug(L_RTP, "[rtp_playout(%s)] -- DELAY PLAYOUT TIME %d --\n", getName(), nDelayCount);
			pNode->addPlayoutTime(HR_TIME_RESOLUTION/m_nFps/2);
			break;
		}
		else {
			//hr_time_t	hrTime = pNode->getPlayoutTime() - pNode->getCreationTime();
			//log_debug(L_RTP, "[rtp_playout(%s)] -- drop node, invalid or timeout (%d ms) --\n",
			//		  	getName(), HR_TIME_TO_MILLISECONDS(hrTime));
			m_lastPlayoutTimestamp = pNode->getTimestamp();
			it = m_arNode.erase(it);
			pNode->release();
			counter_inc(m_nNodeDropped);
		}
	}

#if 0
	it=m_arNode.begin();
	while ( it != m_arNode.end() )  {
		if ( (*it)->getPlayoutTime() < hrNow )  {
			pNode = *it;
			it = m_arNode.erase(it);

			m_lastPlayoutTimestamp = pNode->getTimestamp();

			if ( pNode->completeNode() )  {
				/* Pass completed node to the sink */
				m_pSink->put(pNode);
				pNode->release();
			}
			else {
				//log_debug(L_RTP, "-- DROPPED NODE PLAYOUT --\n");
				pNode->release();
				counter_inc(m_nNodeDropped);
			}
		}
		else {
			it++;
		}
	}
#endif
}

void* CRtpPlayoutBuffer::threadProc(CThread* pThread, void* pData)
{
	while ( atomic_get(&m_bDone) == FALSE ) {
		getInputFrames();
		playout();
		m_cond.lock();
		if ( m_pInputQueue->getLength() == 0 && atomic_get(&m_bDone) == FALSE ) {
			m_cond.waitTimed(getNextWakeupTime());
		}
		m_cond.unlock();
	}

	return NULL;
}

/*
 * Initialise playout buffer
 *
 * 		pSink		outgoing post processor object
 *
 * Return: ESUCCESS, ...
 */
result_t CRtpPlayoutBuffer::init(CVideoSink* pSink)
{
	result_t	nresult;

	shell_assert(m_pSink == 0);
	shell_assert(pSink);

	m_pSink = pSink;
	atomic_set(&m_bDone, FALSE);
	m_rtpSourceInited = FALSE;
	m_lastPlayoutTimestamp = 0;

	nresult = CThread::start(THREAD_CALLBACK(CRtpPlayoutBuffer::threadProc, this), this);
	if ( nresult != ESUCCESS )  {
		log_error(L_RTP, "[rtp_playout(%s)] failed to start thread, result %d\n", getName(), nresult);
		return nresult;
	}

	return ESUCCESS;
}

/*
 * Stop playout buffer
 */
void CRtpPlayoutBuffer::terminate()
{
	flush();
	clear();

	m_cond.lock();
	atomic_set(&m_bDone, TRUE);
	m_cond.wakeup();
	m_cond.unlock();
	CThread::stop();

	m_pSink = 0;
}

/*******************************************************************************
 * Debugging support
 */

int CRtpPlayoutBuffer::validateNodeOrder() const
{
	std::list<CRtpPlayoutNode*>::const_iterator	it;
	int			nInvalid = 0;
	uint64_t	timestamp = 0;

	for(it=m_arNode.begin(); it != m_arNode.end(); it++) {
		if ( it != m_arNode.begin() ) {
			if ( timestamp > (*it)->getTimestamp() ) {
				nInvalid++;
			}
		}
		else {
			timestamp = (*it)->getTimestamp();
		}
	}

	return nInvalid;
}

void CRtpPlayoutBuffer::dumpNodes() const
{
	std::list<CRtpPlayoutNode*>::const_iterator	it;
	int 		n, nInvalidOrder;

	for(n=0, it=m_arNode.begin(); it != m_arNode.end(); it++, n++) {
		(*it)->dump("   ");
	}

	nInvalidOrder = validateNodeOrder();
	if ( nInvalidOrder != 0 )  {
		log_dump("   **************************************\n");
		log_dump("   *** Detected invalid node order %d ***\n", nInvalidOrder);
		log_dump("   **************************************\n");
	}
}

void CRtpPlayoutBuffer::dump(const char* strPref) const
{
	size_t		nodeCount;

	nodeCount = m_arNode.size();

	log_dump("  > %sRTP Playout Buffer: name %s, clock rate: %d, nodes: %u current, %u max, %u dropped\n",
			 strPref, getName(), m_nClockRate, nodeCount,
			 counter_get(m_nNodeMaxCount), counter_get(m_nNodeDropped));

	log_dump("    frames: %d, %u dropped, %u late\n",
			counter_get(m_nFrameCount), counter_get(m_nFrameDropped),
			counter_get(m_nFrameLate));

	m_pInputQueue->dump();
}
