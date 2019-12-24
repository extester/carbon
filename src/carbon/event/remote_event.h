/*
 *  Carbon framework
 *  Remote Events
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 01.08.2016 11:47:14
 *      Initial revision.
 */

#ifndef __CARBON_EVENT_REMOTE_EVENT_H_INCLUDED__
#define __CARBON_EVENT_REMOTE_EVENT_H_INCLUDED__

#include "carbon/net_container.h"
#include "carbon/cstring.h"
#include "carbon/event.h"

#define REMOTE_EVENT_RID_MAX			64
#define REMOTE_EVENT_IDENT				0x6576656E

/*
 * Remove event network exchange packet header
 */
typedef struct {
	uint32_t		ident;				/* Packet identificator, REMOTE_EVENT_IDENT */
	uint32_t		length;				/* Full packet length, bytes */
	event_type_t	type;				/* Event type, EV_xxx */
	uint64_t		pparam;				/* Event PPARAM */
	uint64_t		nparam;				/* Event NPARAM */
	seqnum_t		sessId;				/* Unique request Id */
	uint64_t 		receiver;			/* Target receiver address */
	uint64_t		reply_receiver;		/* Reply received address */
	char			reply_rid[REMOTE_EVENT_RID_MAX];
} __attribute__ ((packed)) remote_event_head_t;


class CRemoteEvent : public CEvent
{
	protected:
		void*				m_pData;
		size_t				m_size;
		CEventReceiver*		m_pReplyReceiver;
		CString				m_strReplyRid;

	public:
		CRemoteEvent() : CEvent(), m_pData(0), m_size(0), m_pReplyReceiver(0)
		{
		}

		CRemoteEvent(event_type_t type, CEventReceiver* pReceiver,  CEventReceiver* pReplyReceiver,
				 seqnum_t sessId, const char* strDesc = EVENT_DESC_DEFAULT) :
			CEvent(type, pReceiver, 0, 0, strDesc),
			m_pData(0),
			m_size(0),
			m_pReplyReceiver(pReplyReceiver)
		{
			setSessId(sessId);
		}

		CRemoteEvent(event_type_t type, CEventReceiver* pReceiver,  CEventReceiver* pReplyReceiver,
					 seqnum_t sessId, const void* pData, size_t size,
					 const char* strDesc = EVENT_DESC_DEFAULT) :
			CEvent(type, pReceiver, 0, 0, strDesc),
			m_pData(0),
			m_size(0),
			m_pReplyReceiver(pReplyReceiver)
		{
			setData(pData, size);
			setSessId(sessId);
		}

		CRemoteEvent(event_type_t type, CEventReceiver* pReceiver, CEventReceiver* pReplyReceiver,
					 seqnum_t sessId, PPARAM pParam, NPARAM nParam,
					 const char* strDesc = EVENT_DESC_DEFAULT) :
			CEvent(type, pReceiver, pParam, nParam, strDesc),
			m_pData(0),
			m_size(0),
			m_pReplyReceiver(pReplyReceiver)
		{
			setSessId(sessId);
		}

		CRemoteEvent(event_type_t type, CEventReceiver* pReceiver, CEventReceiver* pReplyReceiver,
					 seqnum_t sessId, const void* pData, size_t size, PPARAM pParam, NPARAM nParam,
					 const char* strDesc = EVENT_DESC_DEFAULT) :
			CEvent(type, pReceiver, pParam, nParam, strDesc),
			m_pData(0),
			m_size(0),
			m_pReplyReceiver(pReplyReceiver)
		{
			setData(pData, size);
			setSessId(sessId);
		}


	protected:
		virtual ~CRemoteEvent() {
			SAFE_FREE(m_pData);
		}

	public:
		const void* getData() const { return m_pData; }
		size_t getDataSize() const { return m_size; }

		result_t send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr = NETADDR_NULL);
		result_t receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr = NULL);

		const CString& getReplyRid() const { return m_strReplyRid; }
		void setReplyRid(const char* strRid) { m_strReplyRid = strRid; }

		CEventReceiver* getReplyReceiver() const { return m_pReplyReceiver; }

		void copy(const CRemoteEvent& event) {
			CEvent::copy(event);
			setData(event.m_pData, event.m_size);
			m_pReplyReceiver = event.m_pReplyReceiver;
			m_strReplyRid = event.m_strReplyRid;
		}

	private:
		void setData(const void* pData, size_t size);

#if CARBON_DEBUG_DUMP
	public:
		virtual const char* dumpEvent(char* strBuffer, size_t length) const;
		virtual void dump(const char* strPref = "") const;
#endif /* CARBON_DEBUG_DUMP */
};

class CRemoteEventContainer : public CNetContainer
{
	protected:
		dec_ptr<CRemoteEvent>		m_pEvent;

	public:
		CRemoteEventContainer(CRemoteEvent* pEvent) :
			CNetContainer(),
			m_pEvent(pEvent)
		{
			pEvent->reference();
		}

		virtual ~CRemoteEventContainer()
		{
		}

	public:
		virtual void clear() { m_pEvent = 0; }
		virtual CNetContainer* clone();

		CRemoteEvent* getEvent() { m_pEvent->reference(); return m_pEvent; }

		virtual result_t send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr = NETADDR_NULL) {
			result_t	nresult;
			shell_assert((CRemoteEvent*)m_pEvent != 0);
			nresult = m_pEvent->send(socket, hrTimeout, dstAddr);
			return nresult;
		}

		virtual result_t receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr = NULL) {
			result_t	nresult;
			shell_assert((CRemoteEvent*)m_pEvent != 0);
			nresult = m_pEvent->receive(socket, hrTimeout, pSrcAddr);
			return nresult;
		}

#if CARBON_DEBUG_DUMP
	public:
		virtual void dump(const char* strPref = "") const;
		virtual void getDump(char* strBuf, size_t length) const;
#endif /* CARBON_DEBUG_DUMP */
};


#endif /* __CARBON_EVENT_REMOTE_EVENT_H_INCLUDED__ */
