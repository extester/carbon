/*
 *  Carbon framework
 *  Tcp connector Worker thread pool
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.06.2015 18:14:54
 *      Initial revision.
 */

#ifndef __CARBON_TCP_WORKER_H_INCLUDED__
#define __CARBON_TCP_WORKER_H_INCLUDED__

#include "carbon/thread_pool.h"


/*******************************************************************************
 * Tcp Worker pool single item class
 */

class CTcpWorkerPool;

class CTcpWorkerItem : public CThreadPoolItem
{
    public:
		CTcpWorkerItem(CThreadPool* pParent, const char* strName);
        virtual ~CTcpWorkerItem();

    protected:
		CTcpWorkerPool* getParent();

        void notifyRecv(CEventReceiver* pReplyReceiver, result_t nresult,
						   	CNetContainer* pContainer, CSocketRef* pSocket, seqnum_t sessId);
        void notifySend(CEventReceiver* pReplyReceiver, result_t nresult, seqnum_t sessId);

        virtual boolean_t processEvent(CEvent* pEvent);

        virtual void processReceive(CSocketRef* pSocket);
        virtual void processSend(CNetContainer* pContainer, CSocketRef* pSocket,
							CEventReceiver* pReplyReceiver, seqnum_t sessId);
		virtual void processSendLocal(CNetContainer* pContainer, const char* strSocket,
							CEventReceiver* pReplyReceiver, seqnum_t sessId);
        virtual void processIo(CNetContainer* pContainer, const CNetAddr& destAddr,
							CEventReceiver* pReplyReceiver, seqnum_t sessId);
};

/*******************************************************************************
 * TCP Worker pool class
 */

class CTcpConnector;

class CTcpWorkerPool : public CThreadPool
{
    private:
        CTcpConnector*      m_pParent;				/* Parent connector object */

        CNetAddr            m_bindAddr;             /* Sockets bind address, access under lock */
        hr_time_t           m_hrSendTimeout;        /* Send timeout, access under lock */
        hr_time_t           m_hrRecvTimeout;        /* Recv timeout, access under lock */
		hr_time_t			m_hrConnectTimeout;		/* Connect timeout, access under lock */

    public:
        CTcpWorkerPool(size_t nMaxWorkers, CTcpConnector* pParent);
        virtual ~CTcpWorkerPool();

    public:
        /*
         * Worker pool API
         */
        result_t start();
        void stop();

		boolean_t isTerinating() const {
			return m_bTerminated;
		}

        void setTimeouts(hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout)  {
            CAutoLock locker(m_lock);
            m_hrSendTimeout = hrSendTimeout;
            m_hrRecvTimeout = hrRecvTimeout;
        }

		void setConnectTimeout(hr_time_t hrTimeout)  {
			CAutoLock locker(m_lock);
			m_hrConnectTimeout = hrTimeout;
		}

        void getTimeouts(hr_time_t* phrSendTimeout, hr_time_t* phrRecvTimeout) {
            CAutoLock locker(m_lock);
            if ( phrSendTimeout )  {
                *phrSendTimeout = m_hrSendTimeout;
            }
            if ( phrRecvTimeout )  {
                *phrRecvTimeout = m_hrRecvTimeout;
            }
        }

		hr_time_t getConnectTimeout() const {
			CAutoLock locker(m_lock);
			return m_hrConnectTimeout;
		}

        void setBindAddr(const CNetAddr& bindAddr)  {
            CAutoLock locker(m_lock);
            m_bindAddr = bindAddr;
        }

        const CNetAddr& getBindAddr() {
            CAutoLock locker(m_lock);
            return m_bindAddr;
        }

        CEventReceiver* getReceiver() const;

        CTcpWorkerItem* getWorker();

        CTcpConnector* getParent() {
            return m_pParent;
        }

        void statRecv();
        void statSend();
        void statFail();

    protected:
        virtual CThreadPoolItem* createThread(const char* strName);
};

#endif  /* __CARBON_TCP_WORKER_H_INCLUDED__ */
