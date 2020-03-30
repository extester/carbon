/*
 *  Carbon Framework Network Center
 *  Host list
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 30.05.2015 22:43:02
 *      Initial revision.
 */

#ifndef __HOST_LIST_H_INCLUDED__
#define __HOST_LIST_H_INCLUDED__

#include <errno.h>
#include <map>

#include "shell/netaddr.h"
#include "shell/socket.h"
#include "contact/icmp.h"

#include "carbon/carbon.h"
#include "carbon/event/timer.h"
#include "carbon/event/eventloop.h"
#include "host_db.h"


typedef struct
{
    host_id_t       id;
    ip_addr_t       ipAddr;
    char            strName[HOSTLIST_NAME_MAX];

    hr_time_t       hrPing;
    uint32_t        nFailed;
} hostlist_item_t;

#define HOSTLIST_ITEM_NULL      ((hostlist_item_t*)0)

typedef std::map<host_id_t, hostlist_item_t*>                   hostlist_t;
typedef std::map<host_id_t, hostlist_item_t*>::iterator         hostlist_iterator_t;
typedef std::map<host_id_t, hostlist_item_t*>::const_iterator   chostlist_iterator_t;

typedef struct
{
    host_id_t       id;
    ip_addr_t       dstAddr;            /* [IN]  destination address */
    hr_time_t       hrAverage;          /* [OUT] average ping time or ICMP_ITER_FAILED */
} heart_beat_request_t;

class CCenterApp;

class CHostList : public CModule, public CEventReceiver
{
    private:
        CCenterApp*             m_pParent;
        CHostTable*             m_pStoreTable;

        hostlist_t              m_list;
        boolean_t               m_bModified;

        boolean_t               m_bHBPending;
        CTimer*                 m_pHBTimer;

        heart_beat_request_t*   m_pHBTable;
        size_t                  m_nHBCount;
        size_t                  m_nHBTableMax;

    public:
        CHostList(CCenterApp* pParent);
        virtual ~CHostList();

    public:
        virtual result_t init();
        virtual void terminate();

        result_t insertHost(ip_addr_t ipHost, const char* strHost, host_id_t* pId);
        result_t updateHost(host_id_t id, ip_addr_t ipHost, const char* strHost);
        result_t deleteHost(host_id_t id);

        void doHeartBeat();
        void startHeartBeat() {
            doHeartBeat();
            startHBTimer();
        }

        virtual void dump(const char* strPref = 0) const;

    private:
        result_t insertHostTable(host_id_t id, ip_addr_t ipHost, const char* strHost);
        result_t deleteHostTable(host_id_t id);

        void deleteHostTableAll();
        hostlist_item_t* find(host_id_t id);
        hostlist_item_t* find(const CNetHost& ipAddr);

        result_t buildHBTable();
        result_t ensureHBTableSize(size_t count);
        void processHBResult(result_t nresult);

        result_t doHeartBeatPacket(CSocketRef* pSocket);
        void doInsertPacket(CSocketRef* pSocket, CVepContainer* pContainer);
        void doUpdatePacket(CSocketRef* pSocket, CVepContainer* pContainer);
        void doDeletePacket(CSocketRef* pSocket, CVepContainer* pContainer);

        void processPacket(CSocketRef* pSocket, CVepContainer* pContainer);
        virtual boolean_t processEvent(CEvent* pEvent);

        void stopHBTimer();
        void startHBTimer();
        void hbTimerHandler(void* p);

        void dumpTable(const char* strPref = 0) const;
};


#endif /* __HOST_LIST_H_INCLUDED__ */
