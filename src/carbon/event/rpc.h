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
 *  Revision 1.0, 04.04.2013 16:24:02
 *      Initial revision.
 */

#ifndef __CARBON_RPC_H_INCLUDED__
#define __CARBON_RPC_H_INCLUDED__

#include "shell/hr_time.h"

#include "carbon/event.h"
#include "carbon/lock.h"

class CEventLoop;

class CRpc : public CEvent
{
    private:
        CCondition      m_cond;
        hr_time_t       m_hrTimeout;

    protected:
        CRpc(event_type_t eventType, hr_time_t hrTimeout, const char* strName);
        virtual ~CRpc();

    public:
        void rpcPost(CEventReceiver* pReceiver, CEventLoop* pEventLoop);
        result_t rpcCall(CEventReceiver* pReceiver, CEventLoop* pEventLoop);
        void rpcReply();
};

#endif /* __CARBON_RPC_H_INCLUDED__ */
