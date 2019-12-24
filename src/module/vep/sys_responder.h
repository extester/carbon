/*
 *  Carbon/Vep module
 *  VEP Protocol System packets responder
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 02.06.2015 10:16:26
 *      Initial revision.
 */

#ifndef __CARBON_SYS_RESPONDER_H_INCLUDED__
#define __CARBON_SYS_RESPONDER_H_INCLUDED__

#include "shell/socket.h"

#include "carbon/carbon.h"
#include "carbon/event.h"
#include "carbon/event/eventloop.h"
#include "carbon/application.h"
#include "vep/vep_container.h"

class CTcpConnector;
class CEventTcpConnRecv;

class CSysResponder : public CModule
{
    private:
        CApplication*       m_pApp;
        CTcpConnector*      m_pNetConnector;

    public:
        CSysResponder(CApplication* pApp, CTcpConnector* pNetConnector);
        virtual ~CSysResponder();

    public:
        virtual result_t init();
        virtual void terminate();

        result_t processPacket(CEventTcpConnRecv* pPacket);
		virtual void dump(const char* strPref = "") const;

    protected:
        virtual result_t doPacketVersion(CSocketRef* pSocket);
        virtual result_t doPacketMemoryStat(CSocketRef* pSocket);
        virtual result_t doPacketNetConnStat(CSocketRef* pSocket);
        virtual result_t doPacketGetLoggerChannel(CSocketRef* pSocket);
        virtual result_t doPacketLoggerEnable(CSocketRef* pSocket, CVepContainer* pContainer);
};

#endif  /* __CARBON_SYS_RESPONDER_H_INCLUDED__ */
