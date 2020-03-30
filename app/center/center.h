/*
 *  Carbon Framework Network Center
 *  Main module
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 31.05.2015 21:42:40
 *      Initial revision.
 */

#ifndef __CENTER_H_INCLUDED__
#define __CENTER_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/net_connector/tcp_connector.h"
#include "vep/sys_responder.h"
#include "carbon/application.h"
#include "carbon/event.h"

#include "heart_beat.h"
#include "host_list.h"


class CCenterApp : public CApplication
{
    private:
        CTcpConnector*      m_pNetConnector;
        CSysResponder*      m_pSysResponder;
        CHeartBeat*         m_pHeartBeat;
        CHostList*          m_pHostList;

    public:
        CCenterApp(int argc, char* argv[]);
        virtual ~CCenterApp();

    public:
        virtual result_t init();
        virtual void terminate();

        CTcpConnector* getNetConnector() const {
            shell_assert(m_pNetConnector);
            return m_pNetConnector;
        }

        CSysResponder* getSysResponder() const {
            shell_assert(m_pSysResponder);
            return m_pSysResponder;
        }

        CHeartBeat* getHeartBeat() const {
            shell_assert(m_pHeartBeat);
            return m_pHeartBeat;
        }

        CHostList* getHostList() const {
            shell_assert(m_pHostList);
            return m_pHostList;
        }

    protected:
        void processEventPacket(CEventTcpConnRecv* pEventRecv);
        virtual boolean_t processEvent(CEvent* pEvent);

        void initLogger();

};

#endif /* __CENTER_H_INCLUDED__ */
