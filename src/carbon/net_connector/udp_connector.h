/*
 *  Carbon framework
 *  UDP connector module
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 14.04.2017 11:41:28
 *      Initial revision.
 */
/*
 * Purpose:
 *      Asynchronous network UDP I/O operations
 *
 * Constructor:
 *      CUdpConnector(CNetContainer* pRecvTempl, EventReceiver* pParent);
 *          pRecvTempl      sample container for receiving the new containers
 *      	pParent         default container receiver
 *
 *
 * I) Initialisation/Deinitialisation API:
 *    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *      result_t init();
 *          Prepare to perform network I/O operations (listen server is not started);
 *
 *      result_t startListen(const CNetAddr& listenAddr);
 *          Start accepting incoming packets.
 *          listenAddr      address to listen
 *
 *      void stopListen();
 *          Cancel accepting incoming connections.
 *
 *      void terminate();
 *          Cancel all I/O operations and stop all workers (including listen server).
 *
 *
 * II) Settings API (to be called before init()):
 *     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *      void setBindAddr(const CNetAddr&);
 *          Setup source network address for send().
 *
 *      const CNetAddr& getBindAddr() const;
 *          Obtain current source network address.
 *
 *      void setSendTimeout(hr_time_t hrSendTimeout);
 *          Setup send timeout.
 *
 *      hr_time_t getSendTimeout();
 *          Obtain current send timeout.
 *
 *
 * III) Public API:
 *      ~~~~~~~~~~~
 *
 *		result_t send(CNetContainer* pContainer, const CNetAddr& dstAddr,
 *						CEventReceiver* pReplyReceiver = 0, seqnum_t sessId = NO_SEQNUM);
 *			Send a container to the specified destination address.
 *			pContainer			containet to send
 *			destAddr			destination address
 *			pReplyReceiver		result receiver (may be NULL)
 *			sessId				unique session Id
 *
 */

#ifndef __CARBON_UDP_CONNECTOR_H_INCLUDED__
#define __CARBON_UDP_CONNECTOR_H_INCLUDED__

#include "shell/config.h"
#include "shell/netaddr.h"
#include "shell/counter.h"
#include "shell/dec_ptr.h"
#include "shell/thread.h"

#include "carbon/carbon.h"
#include "carbon/module.h"
#include "carbon/event.h"
#include "carbon/net_container.h"


class CEventUdpRecv : public CEvent
{
	private:
		CNetAddr	m_srcAddr;

	public:
		CEventUdpRecv(CEventReceiver* pReceiver, CNetContainer* pContainer,
				   const CNetAddr& srcAddr) :
			CEvent(EV_NETCONN_RECV, pReceiver, (PPARAM)pContainer, 0, "udpRecv"),
			m_srcAddr(srcAddr)
		{
			shell_assert(pContainer);
			pContainer->reference();
		}

		virtual ~CEventUdpRecv()
		{
			CNetContainer*	pContainer = getContainer();
			SAFE_RELEASE(pContainer);
		}

	public:
		virtual CNetContainer* getContainer() const {
			return static_cast<CNetContainer*>(getpParam());
		}

		const CNetAddr& getSrcAddr() const {
			return m_srcAddr;
		}
};


/*
 * Udp Connector module statistic data
 */
typedef struct
{
    counter_t   recv;           		/* Receive requests */
    counter_t   send;           		/* Send requests */
    counter_t   fail;        			/* I/O fails */
} __attribute__ ((packed)) udpconn_stat_t;


/*
 * CUdpConnector
 */
class CUdpConnector : public CModule
{
    protected:
        CEventReceiver*         m_pParent;          /* Received container event receiver */
        dec_ptr<CNetContainer>	m_pRecvTempl;	    /* Receive container template */
		CNetAddr				m_listenAddr;		/* Listen (recv) address */
		CNetAddr				m_bindAddr;			/* Bind (source for send) address */
		CThread					m_thread;			/* Worker thread */
		CSocket					m_socket;			/* Receive socket */
		hr_time_t				m_hrSendTimeout;	/* Send timeout */
		atomic_t				m_bDone;			/* Termination flag */

		udpconn_stat_t          m_stat;             /* Module statistic */

    public:
        CUdpConnector(CNetContainer* pRecvTempl, CEventReceiver* pParent);
        virtual ~CUdpConnector();

    public:
		boolean_t isTerminating() const { return atomic_get(&m_bDone) != 0; }

        /* Statistic functions */
        virtual size_t getStatSize() const {
            return sizeof(m_stat);
        }

        virtual void getStat(void* pBuffer, size_t nSize) const {
            size_t rsize = MIN(nSize, sizeof(m_stat));
            UNALIGNED_MEMCPY(pBuffer, &m_stat, rsize);
        }

        virtual void resetStat();

        void statRecv()			{ counter_inc(m_stat.recv); }
        void statSend()			{ counter_inc(m_stat.send); }
        void statFail()			{ counter_inc(m_stat.fail); }

        /*
         * Public API
         */
        result_t startListen(const CNetAddr& listenAddr);
        void stopListen();

        void setBindAddr(const CNetAddr& bindAddr) {
			m_bindAddr = bindAddr;
        }

        const CNetAddr& getBindAddr() const {
            return m_bindAddr;
        }

        void setSendTimeout(hr_time_t hrTimeout)  {
            m_hrSendTimeout = hrTimeout;
        }

        hr_time_t getSendTimeout() const {
            return m_hrSendTimeout;
        }

		virtual result_t sendSync(CNetContainer* pContainer, const CNetAddr& dstAddr);

		virtual result_t init();
		virtual void terminate();

	protected:
		void notifyReceive(CNetContainer* pContainer, const CNetAddr& srcAddr);
		void* workerThread(CThread* pThread, void* p);


#if CARBON_DEBUG_DUMP
	public:
        virtual void dump(const char* strPref = "") const;
#endif /* CARBON_DEBUG_DUMP */
};

#endif  /* __CARBON_UDP_CONNECTOR_H_INCLUDED__ */
