/*
 *  Carbon framework
 *  Synchronous execution helper base class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 08.06.2015 12:00:27
 *      Initial revision.
 */

#ifndef __CARBON_SYNC_H_INCLUDED__
#define __CARBON_SYNC_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/lock.h"
#include "carbon/event.h"

class CSyncBase
{
	protected:
		seqnum_t		m_sessId;
		dec_ptr<CEvent>	m_pEvent;		/* Reply event */

	public:
		CSyncBase(seqnum_t sessId) :
			m_sessId(sessId)
		{
		}

		virtual ~CSyncBase()
		{
		}

	public:
		seqnum_t getSessId() const { return m_sessId; }

		virtual void attachEvent(CEvent* pEvent) {
			m_pEvent = pEvent;
			m_pEvent->reference();
		}

		virtual result_t processSyncEvent() = 0;
};

#endif /* __CARBON_SYNC_H_INCLUDED__ */
