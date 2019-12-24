/*
 *	Carbon/Network MultiMedia Streaming Module
 *	RTP network receiver pool
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 12.10.2016 10:41:52
 *		Initial revision.
 */

#include "carbon/utils.h"

#include "net_media/rtp_frame_cache.h"
#include "net_media/rtp_playout_buffer.h"
#include "net_media/rtp_receiver_pool.h"

#define RTP_RECEIVE_TIMEOUT			HR_16SEC

/*******************************************************************************
 * CRtpReceiver class
 */

CRtpReceiver::CRtpReceiver(const CNetAddr& netAddr, CRtpReceiverPool* pParent) :
	CThread(HR_0, HR_4SEC),
	m_pParent(pParent),
	m_netAddr(netAddr),

	m_nLastSeq(0xffff),
	m_nFrameCount(ZERO_COUNTER),
	m_nFrameDrop(ZERO_COUNTER),
	m_nFrameErr(ZERO_COUNTER),
	m_nFrameErrMem(ZERO_COUNTER),
	m_nFrameLost(ZERO_COUNTER)
{
	char 	strTmp[64];

	_tsnprintf(strTmp, sizeof(strTmp), "%s-%s", pParent->getName(), m_netAddr.cs());
	CThread::setName(strTmp);

	atomic_set(&m_bDone, FALSE);
	m_arPlayoutBuffer.reserve(4);
}

CRtpReceiver::~CRtpReceiver()
{
	shell_assert(!isRunning());
	shell_assert(m_arPlayoutBuffer.empty());
}

/*
 * Find a playout buffer index in the receiver
 *
 * Return: channel index or -1 if none found
 */
int CRtpReceiver::findChannel(const CRtpPlayoutBuffer* pBuffer, boolean_t bLock) const
{
	int 	index = -1;
	size_t	i, count;

	if ( bLock ) { m_lock.lock(); }

	count = m_arPlayoutBuffer.size();
	for(i=0; i<count; i++)  {
		if ( m_arPlayoutBuffer[i] == pBuffer )  {
			index = (int)i;
			break;
		}
	}

	if ( bLock ) { m_lock.unlock(); }

	return index;
}

/*
 * Insert a channel for receiving RTP frames
 *
 * 		pBuffer			playout buffer of the channel
 */
void CRtpReceiver::insertChannel(CRtpPlayoutBuffer* pBuffer)
{
	CAutoLock	locker(m_lock);
	int			i;

	i = findChannel(pBuffer);
	if ( i < 0 )  {
		m_arPlayoutBuffer.push_back(pBuffer);
	}
	else {
		locker.unlock();
		log_debug(L_RTP, "[rtp_recv(%s)] duplicated playout buffer ignored\n", getName());
	}
}

/*
 * Remove channel from the receiver
 *
 * 		pBuffer			playout buffer of the channel
 */
result_t CRtpReceiver::removeChannel(const CRtpPlayoutBuffer* pBuffer)
{
	CAutoLock	locker(m_lock);
	int			i;
	result_t	nresult = ESUCCESS;

	i = findChannel(pBuffer);
	if ( i >= 0 )  {
		m_arPlayoutBuffer.erase(m_arPlayoutBuffer.begin()+i);
	}
	else {
		nresult = ENOENT;
	}

	return nresult;
}

/*
 * Remove all channels from the receiver
 */
void CRtpReceiver::removeAllChannels()
{
	CAutoLock	locker(m_lock);
	m_arPlayoutBuffer.clear();
}

/*
 * Validate, calculate real frame length (exclude padding) and
 * convert RTP frame header from network byte order to host byte order
 *
 * 		pFrame		frame to convert header
 *
 * Return:
 * 		ESUCCESS	converted successful
 * 		EINVAL		frame is not valid
 */
result_t CRtpReceiver::validateFrame(rtp_frame_t* pFrame) const
{
	rtp_head_t*	pHead = &pFrame->head;
	size_t		hlength = 0;
	result_t	nresult = ESUCCESS;

	/*
	 * Convert frame network-to-host byteorder
	 */
	hlength = sizeof(rtp_head_t);
	if ( pFrame->length >= hlength )  {
		size_t 	cc, i;

		pHead->fields = ntohl(pHead->fields);
		pHead->timestamp = ntohl(pHead->timestamp);
		pHead->ssrc = ntohl(pHead->ssrc);
		cc = RTP_HEAD_CC(pHead->fields);

		if ( cc > 0 )  {
			/* Contributing source id(s) are present */
			hlength += cc*sizeof(uint32_t);
			if ( pFrame->length >= hlength ) {
				for(i=0; i<cc; i++)  {
					pHead->csrc[i] = ntohl(pHead->csrc[i]);
				}
			}
			else {
				nresult = EINVAL;
			}
		}

		if ( nresult == ESUCCESS && RTP_HEAD_EXTENSION(pHead->fields) )  {
			/* Extension header are present */
			if ( pFrame->length >= (hlength+sizeof(uint32_t)) ) {
				uint32_t*	pExtHead = (uint32_t*)(((uint8_t*)&pFrame->head)+hlength);
				uint16_t	extWords, ii;

				*pExtHead = ntohl(*pExtHead);
				extWords = (uint16_t)((*pExtHead)&0xffff);
				hlength += (extWords+1)*sizeof(uint32_t);

				if ( pFrame->length >= hlength ) {
					for (ii=0; ii<extWords; ii++) {
						pExtHead++;
						*pExtHead = ntohl(*pExtHead);
					}
				}
				else {
					nresult = EINVAL;
				}
			}
			else {
				nresult = EINVAL;
			}
		}
	}
	else {
		nresult = EINVAL;
	}

	/*
	 * Correct real packet length by excluding padding
	 */
	if ( nresult == ESUCCESS && RTP_HEAD_PADDING(pHead->fields) )  {
		uint8_t*	pPadCount = ((uint8_t*)&pFrame->head)+pFrame->length - sizeof(uint8_t);

		if ( pFrame->length >= (hlength)+(*pPadCount) ) {
			pFrame->length = pFrame->length - (*pPadCount);
		}
		else {
			nresult = EINVAL;
		}
	}

	/*
	 * Validate packet
	 */
	if ( nresult == ESUCCESS && RTP_HEAD_VERSION(pHead->fields) != RTP_VERSION )  {
		nresult = EINVAL;
	}

	return nresult;
}

/*
 * Put a received rtp frame to the appropriate input queue
 *
 * 		pFrame		received RTP frame pointer
 */
void CRtpReceiver::queueFrame(rtp_frame_t* pFrame)
{
	CAutoLock	locker(m_lock);
	int			nProfile = RTP_HEAD_PAYLOAD_TYPE(pFrame->head.fields);
	size_t		i, count = m_arPlayoutBuffer.size();

	/*
	 * Find and put frame to the it's queue
	 */
	for(i=0; i<count; i++) {
		if ( m_arPlayoutBuffer[i]->getProfile() == nProfile )  {
			m_arPlayoutBuffer[i]->putFrame(pFrame);
			counter_inc(m_nFrameCount);
			return;
		}
	}

	/*
	 * No appropriate playout buffer found
	 */
	pFrame->pOwner->put(pFrame);
	counter_inc(m_nFrameDrop);
}

void* CRtpReceiver::threadProc(CThread* pThread, void* pData)
{
	rtp_frame_t*	pFrame;
	CNetAddr		srcAddr;
	result_t		nresult;

	shell_assert(m_socket.isOpen());

	while ( atomic_get(&m_bDone) == FALSE )  {
		pFrame = m_pParent->getCacheFrame();
		if ( pFrame != RTP_FRAME_NULL )  {
			pFrame->length = sizeof(pFrame->head)+sizeof(pFrame->__buffer__);
			nresult = m_socket.receive(&pFrame->head, &pFrame->length, 0,
									   RTP_RECEIVE_TIMEOUT, &srcAddr);
			if ( nresult == ESUCCESS )  {
				pFrame->hrArriveTime = hr_time_now();
				nresult = validateFrame(pFrame);
				if ( nresult == ESUCCESS )  {
					uint16_t	seq = RTP_HEAD_SEQUENCE(pFrame->head.fields);

					if ( m_nLastSeq != 0xffff )  {
						m_nLastSeq++;
						if ( seq != m_nLastSeq ) {
							//log_error(L_RTP, "[rtp_recv(%s) *** WRONG SEQNUM %u => %u (%d) ****\n",
							//		  	getName(), m_nLastSeq, seq, (int)(seq-m_nLastSeq));
							counter_add(m_nFrameLost, (int)(seq-m_nLastSeq));
							m_nLastSeq = seq;
						}
					}
					else {
						m_nLastSeq = seq;
					}

					queueFrame(pFrame);
				}
				else {
					//log_error(L_RTP, "[rtp_recv(%s)] -- DROP INVALID FRAME --\n", getName());
					pFrame->pOwner->put(pFrame);
					counter_inc(m_nFrameErr);
				}
			}
			else {
				//log_error(L_RTP, "[rtp_recv(%s)] -- RECEIVE FAILED, result %d --\n", getName(), nresult);
				pFrame->pOwner->put(pFrame);
				if ( nresult != ETIMEDOUT && nresult != ECANCELED )  {
					sleep_ms(100);
				}
			}
		}
		else {
			//log_error(L_RTP, "[rtp_recv(%s)] -- NO FREE FRAMES --\n", getName());
			counter_inc(m_nFrameErrMem);
			sleep_ms(100);
		}
	}

	return NULL;
}

/*
 * Start receiving a RTP stream
 *
 * 		bindAddr		network address to receive
 *
 * Return: ESUCCESS, ...
 */
result_t CRtpReceiver::init()
{
	result_t	nresult;

	shell_assert(!CThread::isRunning());
	shell_assert(m_netAddr.isValid());

	atomic_set(&m_bDone, FALSE);

	log_debug(L_RTP, "[rtp_recv(%s)] receiving %s\n", getName(), m_netAddr.cs());

	m_socket.breakerEnable();
	nresult = m_socket.open(m_netAddr, SOCKET_TYPE_UDP);
	if ( nresult != ESUCCESS )  {
		return nresult;
	}

	nresult = CThread::start(THREAD_CALLBACK(CRtpReceiver::threadProc, this));
	if ( nresult != ESUCCESS )  {
		m_socket.close();
	}

	return  nresult;
}

/*
 * Stop receiving a RTP stream
 */
void CRtpReceiver::terminate()
{
	atomic_set(&m_bDone, TRUE);
	m_socket.breakerBreak();
	CThread::stop();
	m_socket.close();

#if DEBUG
	if ( logger_is_enabled(LT_DEBUG|L_RTP) ) {
		if ( counter_get(m_nFrameCount) > 0 || counter_get(m_nFrameDrop) > 0 ||
			counter_get(m_nFrameLost) > 0 || counter_get(m_nFrameErr) > 0 ||
			counter_get(m_nFrameErrMem) > 0 )  {
			dump("ERRORS IN ");
		}
	}
#endif
}

/*******************************************************************************
 * Debugging support
 */

void CRtpReceiver::dump(const char* strPref) const
{
	CAutoLock	locker(m_lock);
	size_t		count = m_arPlayoutBuffer.size();

	log_dump("    %sReceiver: name %s, %u buffer(s), ip: %s, Frames: %d success, %d dropped, "
			 "%d lost, %d invalid, %d mem failed\n",
			 strPref, getName(), count, m_netAddr.cs(),
			 counter_get(m_nFrameCount), counter_get(m_nFrameDrop),
			 counter_get(m_nFrameLost),  counter_get(m_nFrameErr),
			 counter_get(m_nFrameErrMem));
}


/*******************************************************************************
 * CRtpReceiverPool class
 */

CRtpReceiverPool::CRtpReceiverPool(const char* strName, int nMaxCacheFrames) :
	CObject(strName),
	m_bReceiving(FALSE)
{
	m_pFrameCache = new CRtpFrameCache(nMaxCacheFrames);
}

CRtpReceiverPool::~CRtpReceiverPool()
{
	shell_assert_ex(m_arReceiver.empty(), "[rtp_recv_pool(%s)] where are %u receiver(s) exist",
						getName(), m_arReceiver.size());
	SAFE_DELETE(m_pFrameCache);
}

/*
 * Find receiver for the specified network address
 *
 * 		netAddr		network address to serach receiver
 *
 * Return: receiver index or -1 if none found
 */
int CRtpReceiverPool::findChannel(const CNetAddr& netAddr) const
{
	size_t	i, count = m_arReceiver.size();
	int 	index = -1;

	for(i=0; i<count; i++)  {
		if ( m_arReceiver[i]->getNetAddr() == netAddr )  {
			index = (int)i;
			break;
		}
	}

	return index;
}

/*
 * Insert an RTP channel for receiving
 *
 * 		pBuffer		channel's playout buffer to insert
 * 		netAddr		channel's address to insert
 */
void CRtpReceiverPool::insertChannel(CRtpPlayoutBuffer* pBuffer, const CNetAddr& netAddr)
{
	CAutoLock		locker(m_lock);
	int				index;
	CRtpReceiver*	pReceiver = 0;

	index = findChannel(netAddr);
	if ( index >= 0 ) {
		pReceiver = m_arReceiver[index];
	}
	else {
		pReceiver = new CRtpReceiver(netAddr, this);
		m_arReceiver.push_back(pReceiver);
	}

	pReceiver->insertChannel(pBuffer);
}

/*
 * Remove channel from receiver
 *
 * 		pBuffer			channel's playout buffer to remove
 */
void CRtpReceiverPool::removeChannel(const CRtpPlayoutBuffer* pBuffer)
{
	CAutoLock		locker(m_lock);
	size_t			i, count = m_arReceiver.size();
	result_t		nr;

	for(i=0; i<count; i++) {
		nr = m_arReceiver[i]->removeChannel(pBuffer);
		if ( nr == ESUCCESS )  {
			if ( m_arReceiver[i]->getSize() == 0 )  {
				m_arReceiver[i]->terminate();
				SAFE_DELETE(m_arReceiver[i]);
				m_arReceiver.erase(m_arReceiver.begin()+i);
			}
			break;
		}
	}
}

/*
 * Remove all channels from receiver pool
 */
void CRtpReceiverPool::removeAllChannels()
{
	CAutoLock		locker(m_lock);
	size_t			i, count = m_arReceiver.size();

	for(i=0; i<count; i++) {
		m_arReceiver[i]->removeAllChannels();
		delete m_arReceiver[i];
	}

	m_arReceiver.clear();
}

int CRtpReceiverPool::findChannel(const CRtpPlayoutBuffer* pBuffer) const
{
	size_t	i, count = m_arReceiver.size();
	int 	index = -1, ii;

	for(i=0; i<count; i++) {
		ii = m_arReceiver[i]->findChannel(pBuffer);
		if ( ii >= 0 )  {
			index = (int)i;
			break;
		}
	}

	return index;
}

void CRtpReceiverPool::getStat(const CRtpPlayoutBuffer* pBuffer, int* pnFrameLost) const
{
	CAutoLock	locker(m_lock);
	int 		index;

	*pnFrameLost = 0;

	index = findChannel(pBuffer);
	if ( index >= 0 )  {
		m_arReceiver[index]->getStat(pnFrameLost);
	}
}

/*
 * Initialise receiver pool
 *
 * Return:
 * 		ESUCCESS		al least one receiver is initialised successful
 * 		...				none receivers were initialised
 */
result_t CRtpReceiverPool::init()
{
	CAutoLock		locker(m_lock);
	size_t			i, count = m_arReceiver.size();
	int 			nFail = 0, nSuccess = 0;
	result_t		nresult = ESUCCESS, nr;

	for(i=0; i<count; i++) {
		nr = m_arReceiver[i]->init();
		if ( nr == ESUCCESS )  {
			nSuccess++;
		}
		else {
			nFail++;
		}
	}

	nresult = nSuccess > 0 ? ESUCCESS : nresult;
	m_bReceiving = nresult == ESUCCESS;

	return nresult;
}

/*
 * Stop all receivers
 */
void CRtpReceiverPool::terminate()
{
	CAutoLock		locker(m_lock);
	size_t			i, count = m_arReceiver.size();

	for(i=0; i<count; i++) {
		m_arReceiver[i]->terminate();
	}

	m_bReceiving = FALSE;
}

/*******************************************************************************
 * Debugging support
 */

void CRtpReceiverPool::dump(const char* strPref) const
{
	CAutoLock		locker(m_lock);
	size_t			i, count = m_arReceiver.size();

	log_dump("*** %sRTP receiver pool(%s): %d receiver(s), %s\n",
			 strPref, getName(), count, m_bReceiving ? "Receiving" : "STOPPED");

	for(i=0; i<count; i++)  {
		m_arReceiver[i]->dump();
	}

	m_pFrameCache->dump();
}
