/*
 *  Carbon framework
 *  Event base class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.06.2015 11:29:16
 *      Initial revision.
 */

#ifndef __CARBON_EVENT_EVENT_H_INCLUDED__
#define __CARBON_EVENT_EVENT_H_INCLUDED__

#include "shell/config.h"
#include "shell/types.h"
#include "shell/hr_time.h"
#include "shell/ref_object.h"
#include "shell/object.h"

#include "carbon/carbon.h"
#include "carbon/lock.h"

#if CARBON_DEBUG_TRACK_OBJECT
#include "shell/track_object.h"
#endif /* CARBON_DEBUG_TRACK_OBJECT */

typedef uint16_t                    event_type_t;
typedef ptr_t						PPARAM;
typedef natural_t					NPARAM;

/*******************************************************************************
 * Event processing class
 */
class CEvent;
class CEventLoop;

class CEventReceiver : public CObject, public CListItem
{
    protected:
        CEventLoop*     m_pEventLoop;

    public:
        CEventReceiver(CEventLoop* pOwnerLoop, const char* strName);
		explicit CEventReceiver(const char* strName);
        virtual ~CEventReceiver();

    public:
        CEventLoop* getEventLoop() const { return m_pEventLoop; }

        virtual boolean_t processEvent(CEvent* pEvent) = 0;
};

/*******************************************************************************
 * Base event class
 */

#define EVENT_MULTICAST             ((CEventReceiver*)0)
#define EVENT_DESC_DEFAULT          "<nodesc>"

#if CARBON_DEBUG_TRACK_OBJECT
#define __CEvent_PARENT				,public CTrackObject<CEvent>
#else /* CARBON_DEBUG_TRACK_OBJECT */
#define __CEvent_PARENT
#endif /* CARBON_DEBUG_TRACK_OBJECT */

class CEvent : public CListItem, public CRefObject __CEvent_PARENT
{
    protected:
        event_type_t        m_type;				/* Event type */
        CEventReceiver*     m_pReceiver;		/* Destination object or NULL */
        hr_time_t           m_hrTime;		    /* Send time */
        PPARAM				m_pParam;			/* Optional parameter1 (pointer) */
        NPARAM				m_nParam;			/* Optional parameter2 (integer) */
        seqnum_t            m_sessId;           /* Optional unique event number */
        char				m_strDesc[CARBON_OBJECT_NAME_LENGTH];	/* Event description */

    public:
        CEvent() :
			CListItem(),
			CRefObject()/*,
			CTrackObject<CEvent>()*/
        {
            init((event_type_t)0, EVENT_MULTICAST, 0, 0, EVENT_DESC_DEFAULT);
        }

        CEvent(event_type_t type, CEventReceiver* pReceiver, PPARAM pParam,
                NPARAM nParam, const char* strDesc = EVENT_DESC_DEFAULT) :
            CListItem(),
            CRefObject()/*,
            CTrackObject<CEvent>()*/
        {
            shell_assert(strDesc);
            init(type, pReceiver, pParam, nParam, strDesc);
        }

        void* operator new(size_t size) throw();
        void* operator new[](size_t size) throw();
        void operator delete(void* pData);
        void operator delete[](void* pData);

    protected:
        virtual ~CEvent()
        {
        }

    public:
        event_type_t getType() const { return m_type; }
        hr_time_t getTime() const { return m_hrTime; }

        CEventReceiver* getReceiver() const { return m_pReceiver; }
        void setReceiver(CEventReceiver* pReceiver) { m_pReceiver = pReceiver; }

        void setSessId(seqnum_t sessId) { m_sessId = sessId; }
        seqnum_t getSessId() const { return m_sessId; }

        PPARAM getpParam() const { return m_pParam; }
        NPARAM getnParam() const { return m_nParam; }

        void copy(const CEvent& event) {
            m_type = event.m_type;
            m_pReceiver = event.m_pReceiver;
            m_hrTime = event.m_hrTime;
            m_pParam = event.m_pParam;
            m_nParam = event.m_nParam;
            m_sessId = event.m_sessId;
            copyString(m_strDesc, event.m_strDesc, sizeof(m_strDesc));
        }

    protected:
        void setDescription(const char* strDesc)
        {
            copyString(m_strDesc, strDesc, sizeof(m_strDesc));
        }

        void init(event_type_t type, CEventReceiver* pReceiver,
                  PPARAM pParam, NPARAM nParam, const char* strDesc)
        {
            m_type = type;
            m_pReceiver = pReceiver;
            m_hrTime = hr_time_now();
            m_pParam = pParam;
            m_nParam = nParam;
            setDescription(strDesc);
            m_sessId = NO_SEQNUM;
        }

    public:
#if CARBON_DEBUG_EVENT
		void getEventName(char* strBuffer, size_t length) const;
#else /* CARBON_DEBUG_EVENT */
		void getEventName(char* strBuffer, size_t length) const {
			if ( length > 0 ) { *strBuffer = '\0'; }
		}
#endif /* CARBON_DEBUG_EVENT */

#if CARBON_DEBUG_DUMP
        virtual const char* dumpEvent(char* strBuffer, size_t length) const;
	    virtual void dump(const char* strPref = "") const;
#else /* CARBON_DEBUG_DUMP */
		virtual const char* dumpEvent(char* strBuffer, size_t length) const {
			if ( length > 0 ) { *strBuffer = '\0'; }
			return strBuffer;
		}
    	virtual void dump(const char* strPref = "") const { UNUSED(strPref); }
#endif /* CARBON_DEBUG_DUMP */
};


/*
 * Event with payload data
 */
template <typename T, event_type_t type>
class CEventT : public CEvent
{
    private:
        T		m_Data;

    public:
        CEventT(const T* pData, CEventReceiver* pReceiver, PPARAM pParam,
                                    NPARAM nParam, const char* strDesc) :
            CEvent(),
            m_Data(*pData)
        {
            init(type, pReceiver, pParam, nParam, strDesc);
        }

        CEventT(const T* pData, CEventReceiver* pReceiver, const char* strDesc) :
            CEvent(),
            m_Data(*pData)
        {
            init(type, pReceiver, 0, 0, strDesc);
        }

        CEventT(const T* pData, CEventReceiver* pReceiver) :
            CEvent(),
            m_Data(*pData)
        {
            init(type, pReceiver, 0, 0, EVENT_DESC_DEFAULT);
        }

        virtual ~CEventT() {}

    public:
        const T* getData() const { return &m_Data; }
};

#endif /* __CARBON_EVENT_EVENT_H_INCLUDED__ */
