/*
 *  Carbon Framework Network Center
 *  Hosts checker
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 02.06.2015 12:36:46
 *      Initial revision.
 */

#ifndef __HEART_BEAT_H_INCLUDED__
#define __HEART_BEAT_H_INCLUDED__

#include "shell/netaddr.h"

#include "carbon/event/eventloop.h"
#include "carbon/carbon.h"

#include "host_list.h"

class CCenterApp;

class CHeartBeat : public CModule, public CEventLoopThread, public CEventReceiver
{
    private:
        CCenterApp*     m_pParent;
        CNetAddr        m_srcAddr;

    public:
        CHeartBeat(CCenterApp* pParent, const CNetAddr& srcAddr);
        virtual ~CHeartBeat();

    public:
        virtual result_t init();
        virtual void terminate();

		virtual void dump(const char* strPref = 0) const {}

    private:
        result_t runPing(heart_beat_request_t* pTable, size_t count);
        void processRequest(heart_beat_request_t* pTable, size_t count);
        virtual boolean_t processEvent(CEvent* pEvent);
};

#endif  /* __HEART_BEAT_H_INCLUDED__ */
