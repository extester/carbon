/*
 *  Carbon framework
 *  TCP Connector Listening server
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.06.2015 16:28:01
 *      Initial revision.
 */

#ifndef __CARBON_TCP_LISTEN_H_INCLUDED__
#define __CARBON_TCP_LISTEN_H_INCLUDED__

#include "carbon/thread.h"
#include "carbon/event.h"
#include "carbon/tcp_server.h"

class CEventTcpConnDoConnect : public CEvent
{
    public:
		CEventTcpConnDoConnect(CEventReceiver* pReceiver, CSocketRef* pSocket) :
            CEvent(EV_NETCONN_DO_CONNECT, pReceiver, (PPARAM)0, (NPARAM)pSocket)
        {
            shell_assert(pSocket);
            pSocket->reference();
        }

        virtual ~CEventTcpConnDoConnect()
        {
            CSocketRef*     pSocket = getSocket();
            SAFE_RELEASE(pSocket);
        }

    public:
        CSocketRef* getSocket() const {
            return reinterpret_cast<CSocketRef*>(getnParam());
        }
};

class CTcpConnector;

class CTcpListenServer : public CTcpServer
{
    private:
        CTcpConnector*      m_pParent;          /* Parent module */
        CThread             m_thServer;         /* Thread to run within */

    public:
        CTcpListenServer(CTcpConnector* pParent);
        virtual ~CTcpListenServer();

    public:
        /*
         * Listen server API
         */
        virtual result_t start();
        virtual void stop();

        //const CNetAddr& getListenAddr() const {
        //    return m_listenAddr;
        //}

        //virtual void setListenAddr(const CNetAddr& listenAddr) {
        //    CTcpServer::setListenAddr(listenAddr);
        //}

        //virtual void setListenAddr(const char* strSocket) {
        //    CTcpServer::setListenAddr(strSocket);
        //}

    private:
        virtual result_t processClient(CSocketRef* pSocket);
        void* thread(CThread* pThread, void* pData);
};

#endif  /* __CARBON_TCP_LISTEN_H_INCLUDED__ */
