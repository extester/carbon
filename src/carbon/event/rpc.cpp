/*
 *  Carbon framework
 *  Remote Procedure Call
 *
 *  Copyright (c) 2013 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 04.04.2013 16:23:47
 *      Initial revision.
 */

#include "shell/hr_time.h"

#include "carbon/logger.h"
#include "carbon/event/eventloop.h"
#include "carbon/event/rpc.h"


/*******************************************************************************
 * RPC base class
 */

CRpc::CRpc(event_type_t type, hr_time_t hrTimeout, const char* strName) :
	CEvent(type, EVENT_MULTICAST, (PPARAM)0, (NPARAM)0, strName),
    m_hrTimeout(hrTimeout)
{
}

CRpc::~CRpc()
{
}

/*
 * Send command and immediate return without waiting
 *
 *      destination         command destination object
 *      pEventLoop          event loop destination
 */
void CRpc::rpcPost(CEventReceiver* pReceiver, CEventLoop* pEventLoop)
{
    pEventLoop->sendEvent(this, pReceiver);
}

/*
 * Send command to destination and wait the response
 *
 *      destination         command destination object
 *      pEventLoop          event loop destination
 *
 * Return: 0, ETIMEDOUT
 */
result_t CRpc::rpcCall(CEventReceiver* pReceiver, CEventLoop* pEventLoop)
{
	result_t	nresult;

    m_cond.lock();
    reference();
    pEventLoop->sendEvent(this, pReceiver);
    nresult = this->m_cond.waitTimed(hr_time_now() + m_hrTimeout);
    this->m_cond.unlock();

    if ( nresult != ESUCCESS )  {
    	log_debug(L_GEN, "RPC call failed, event: %d, error: %d\n",
                        getType(), nresult);
    }

    return nresult;
}

/*
 * RPC object reply notification
 */
void CRpc::rpcReply()
{
    this->m_cond.lock();
    this->m_cond.wakeup();
    this->m_cond.unlock();
}
