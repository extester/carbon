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

#include <new>
#include <stdexcept>

#include "shell/utils.h"
#include "shell/file.h"

#include "carbon/carbon.h"
#include "vep/vep.h"

#include "event.h"
#include "center.h"
#include "heart_beat.h"
#include "packet_center.h"
#include "host_list.h"

#define HOST_PING_FAILED_MAX        3
#define HOST_PING_INTERVAL          HR_5SEC


/*******************************************************************************
 * CHostList class
 */

CHostList::CHostList(CCenterApp* pParent) :
    CModule("HostList"),
    CEventReceiver(appMainLoop(), "HostList"),
    m_pParent(pParent),
    m_pStoreTable(0),
    m_bModified(FALSE),
    m_bHBPending(FALSE),
    m_pHBTimer(0),
    m_pHBTable(0),
    m_nHBCount(0),
    m_nHBTableMax(0)
{
}

CHostList::~CHostList()
{
    deleteHostTableAll();
    SAFE_FREE(m_pHBTable);
}

/*
 * Insert a new host to the table
 *
 *      id          host ID
 *      ipHost      IP address
 *      strHost     host name
 *
 * Return: ESUCCESS, EEXIST, ENOMEM
 */
result_t CHostList::insertHostTable(host_id_t id, ip_addr_t ipHost, const char* strHost)
{
    hostlist_item_t*    pItem = HOSTLIST_ITEM_NULL;
    result_t            nresult = ESUCCESS;

    shell_assert(id);
    shell_assert(strHost);

    pItem = find(id);
    if ( pItem != HOSTLIST_ITEM_NULL )  {
        log_debug(L_GEN, "[hostlist] host id=%u already exists\n", id);
        return EEXIST;
    }

    try {
        pItem = new hostlist_item_t;
        pItem->id = id;
        pItem->ipAddr = ipHost;
        copyString(pItem->strName, strHost, sizeof(pItem->strName));
        pItem->hrPing = ICMP_ITER_NOTSET;
        pItem->nFailed = 0;
        m_list[id] = pItem;
    }
    catch(std::bad_alloc& exc)  {
        log_error(L_GEN, "[hostlist] failed to allocate memory\n");
        SAFE_DELETE(pItem);
        nresult = ENOMEM;
    }

    return nresult;
}

/*
 * Delete host from the list
 *
 *      id          host ID to delete
 *
 * Return: ESUCCESS, ENOENT
 */
result_t CHostList::deleteHostTable(host_id_t id)
{
    hostlist_iterator_t it;
    result_t            nresult = ESUCCESS;

    it = m_list.find(id);
    if ( it != m_list.end() )  {
        delete it->second;
        m_list.erase(it);
    }
    else {
        log_debug(L_GEN, "[hostlist] host id=%u%s is not found\n", id);
        nresult = ENOENT;
    }

    return nresult;
}


/*
 * Remove all items from the host table
 */
void CHostList::deleteHostTableAll()
{
    hostlist_iterator_t     it;

    for(it=m_list.begin(); it != m_list.end(); it++) {
        delete it->second;
    }

    m_list.clear();
}

/*
 * Find host by ID
 *
 *      id      host id to find
 *
 * Return: item address or HOSTLIST_ITEM_NULL
 */
hostlist_item_t* CHostList::find(host_id_t id)
{
    hostlist_item_t*        pResult = HOSTLIST_ITEM_NULL;
    hostlist_iterator_t     it;

    it = m_list.find(id);
    if ( it != m_list.end() )  {
        pResult = it->second;
    }

    return pResult;
}

/*
 * Find host by ID
 *
 *      id      host id to find
 *
 * Return: item address or HOSTLIST_ITEM_NULL
 */
hostlist_item_t* CHostList::find(const CNetHost& ipAddr)
{
    ip_addr_t               ip = ipAddr;
    hostlist_iterator_t     it;
    hostlist_item_t*        pResult = HOSTLIST_ITEM_NULL;

    for(it=m_list.begin(); it != m_list.end(); it++) {
        if ( it->second->ipAddr == ip )  {
            pResult = it->second;
            break;
        }
    }

    return pResult;
}

/*
 * Insert a new host
 *
 *      ipHost          host IP address
 *      strHost         host name
 *      pId             host ID [out]
 *
 * Return: ESUCCESS, EINVAL, EEXIST, ...
 */
result_t CHostList::insertHost(ip_addr_t ipHost, const char* strHost, host_id_t* pId)
{
    CNetHost            host(ipHost);
    hostlist_item_t*    pItem;
    host_id_t           id;
    result_t            nresult;

    if ( ipHost == 0 || strHost == 0 || *strHost == '\0' )  {
        log_error(L_GEN, "[hostlist] invalid parameters\n");
        return EINVAL;
    }

    pItem = find(host);
    if ( pItem != HOSTLIST_ITEM_NULL )  {
        log_error(L_GEN, "[hostlist] duplicated host ip: %s ignored\n",
                  (const char*)host);
        return EEXIST;
    }

    nresult = m_pStoreTable->insertHost(ipHost, strHost, &id);
    if ( nresult == ESUCCESS )  {
        nresult = insertHostTable(id, ipHost, strHost);
        if ( nresult == ESUCCESS)  {
            *pId = id;
            m_bModified = TRUE;
            log_debug(L_GEN, "[hostlist] inserted new host: %s, ip: %s, id: %lu\n",
                      strHost, (const char*)host, id);
        }
        else {
            m_pStoreTable->deleteHost(id);
        }
    }
    else {
        log_debug(L_GEN, "[hostlist] failed to insert host %s to DB, result: %d\n", strHost, nresult);
    }

    return nresult;
}


/*
 * Update host parameters
 *
 *      id              host Id to update
 *      ipHost          host IP address (may be 0)
 *      strHost         host name (may be NULL)
 *
 * Return: ESUCCESS, ENOENT, ...
 */
result_t CHostList::updateHost(host_id_t id, ip_addr_t ipHost, const char* strHost)
{
    hostlist_item_t*    pItem;
    result_t            nresult;

    pItem = find(id);
    if ( pItem == HOSTLIST_ITEM_NULL )  {
        log_error(L_GEN, "[hostlist] no host ID=%u found\n", id);
        return ENOENT;
    }

    nresult = m_pStoreTable->updateHost(id, ipHost, strHost);
    if ( nresult == ESUCCESS )  {
        if ( ipHost != 0 )  {
            pItem->ipAddr = ipHost;
        }
        if ( strHost != 0 )  {
            copyString(pItem->strName, strHost, sizeof(pItem->strName));
        }

        m_bModified = TRUE;
        log_debug(L_GEN, "[hostlist] updated host ID=%u\n", id);
    }
    else {
        log_debug(L_GEN, "[hostlist] failed to update host ID=%u, result: %d\n", id, nresult);
    }

    return nresult;
}

/*
 * Delete host
 *
 *      id          host Id to delete
 *
 * Return: ESUCCESS, ENOENT, ...
 */
result_t CHostList::deleteHost(host_id_t id)
{
    hostlist_item_t*    pItem;
    result_t            nresult;

    pItem = find(id);
    if ( pItem == HOSTLIST_ITEM_NULL )  {
        log_error(L_GEN, "[hostlist] no host ID=%u found\n", id);
        return ENOENT;
    }

    nresult = m_pStoreTable->deleteHost(id);
    if ( nresult != ESUCCESS )  {
        log_warning(L_GEN, "[hostlist] failed to delete host ID=%u from DB, result: %d\n", id, nresult);
    }

    deleteHostTable(id);
    return ESUCCESS;
}

/*
 * Reallocate heartbeat table to contain at least 'count' items
 *
 *      count       minimum table size in items
 *
 * Return: ESUCECSS, ENOMEM
 */
result_t CHostList::ensureHBTableSize(size_t count)
{
    result_t    nresult = ESUCCESS;

    if ( count > m_nHBTableMax )  {
        size_t  allocSize, allocCount;
        ptr_t   p;

        allocCount = (count + 63)&(~63);
        allocSize = allocCount * sizeof(heart_beat_request_t);
        allocSize = PAGE_ALIGN(allocSize);
        allocCount = allocSize/sizeof(heart_beat_request_t);

        log_debug(L_GEN, "[hostlist] allocating hearbeat table: %u items (%u bytes)\n",
                  allocCount, allocSize);
        p = ::realloc(m_pHBTable, allocSize);
        if ( p )  {
            m_pHBTable = (heart_beat_request_t*)p;
            m_nHBTableMax = allocCount;
        }
        else {
            log_error(L_GEN, "[hotslist] failed to allocate memory %d bytes\n", allocSize);
            nresult = ENOMEM;
        }
    }

    return nresult;
}

/*
 * Create a heartbeat table
 *
 * Return: ESUCCESS, ENOMEM
 */
result_t CHostList::buildHBTable()
{
    hostlist_iterator_t     it;
    size_t                  count, n;
    result_t                nresult;

    count = m_list.size();

    nresult = ensureHBTableSize(count);
    if ( nresult == ESUCCESS )  {
        for(n=0, it=m_list.begin(); it != m_list.end(); it++, n++) {
            m_pHBTable[n].id = it->second->id;
            m_pHBTable[n].dstAddr = it->second->ipAddr;
            m_pHBTable[n].hrAverage = HR_0;
        }

        m_nHBCount = count;
        m_bModified = FALSE;
    }

    return nresult;
}

void CHostList::doHeartBeat()
{
    shell_assert(!m_bHBPending);
    if ( m_bModified )  {
        buildHBTable();
    }

    if ( m_nHBCount > 0 )  {
        CEvent*     pEvent;

        pEvent = new CEvent(EV_HEARTBEAT_REQUEST, m_pParent->getHeartBeat(),
                            (PPARAM)m_pHBTable, (NPARAM)m_nHBCount, "");
        appSendEvent(pEvent);
        m_bHBPending = TRUE;
    }
}

void CHostList::processHBResult(result_t nresult)
{
    m_bHBPending = FALSE;

    if ( nresult == ESUCCESS )  {
        if ( m_nHBCount > 0 )  {
            heart_beat_request_t*   pReq;
            hostlist_item_t*        pHost;
            size_t                  i;

            shell_assert(m_pHBTable);

            for(i=0; i<m_nHBCount; i++)  {
                pReq = &m_pHBTable[i];
                pHost = find(pReq->id);

                if ( pHost != HOSTLIST_ITEM_NULL ) {
                    if ( pReq->hrAverage != ICMP_ITER_FAILED) {
                        pHost->hrPing  = pReq->hrAverage;
                        pHost->nFailed = 0;
                    }
                    else {
                        pHost->nFailed++;
                        if ( pHost->nFailed == HOST_PING_FAILED_MAX ) {
                            pHost->hrPing = ICMP_ITER_FAILED;
                        }
                    }
                }
            }
        }
    }
}

result_t CHostList::doHeartBeatPacket(CSocketRef* pSocket)
{
    dec_ptr<CVepContainer>  containerPtr = new CVepContainer();
    hostlist_iterator_t     it;
    result_t                nresult;

    containerPtr->create(VEP_CONTAINER_CENTER, CENTER_PACKET_HEARTBEAT_REPLY);

    nresult = ESUCCESS;
    for(it=m_list.begin(); it!=m_list.end(); it++)  {
        heartbeat_t     hb;

        hb.id = it->second->id;
        hb.ipAddr = it->second->ipAddr;
        copyString(hb.strName, it->second->strName, sizeof(hb.strName));
        hb.hrPing = it->second->hrPing;

        nresult = containerPtr->insertData(&hb, sizeof(hb));
        if ( nresult != ESUCCESS )  {
            log_error(L_GEN, "[hostlist] failed to create container, result: %d\n", nresult);
            break;
        }
    }

    if ( nresult == ESUCCESS )  {
        nresult = m_pParent->getNetConnector()->send(containerPtr, pSocket);
        if ( nresult != ESUCCESS )  {
            log_error(L_GEN, "[hostlist] failed to send reply, result: %d\n", nresult);
        }
    }

    return nresult;
}

void CHostList::doInsertPacket(CSocketRef* pSocket, CVepContainer* pContainer)
{
    dec_ptr<CVepContainer>          replyContainerPtr;
    center_packet_insert_reply_t    reply;
    union centerPacketArgs*         args;
    host_id_t                       id;
    result_t                        nresult;

    args = (union centerPacketArgs*)pContainer->getPacketHead();

	log_debug(L_GEN, "--- inserting host: IP: %s, NAME: %s\n",
          (const char*)CNetHost(args->insert.ipAddr), args->insert.strName);

    nresult = insertHost(args->insert.ipAddr, args->insert.strName, &id);
    if ( nresult == ESUCCESS ) {
        reply.id = id;
    }
    reply.nresult = nresult;

    replyContainerPtr = new CVepContainer(VEP_CONTAINER_CENTER);
    replyContainerPtr->insertPacket(CENTER_PACKET_INSERT_REPLY, &reply, sizeof(reply));

    nresult = m_pParent->getNetConnector()->send(replyContainerPtr, pSocket);
    if ( nresult != ESUCCESS )  {
        log_error(L_GEN, "[hostlist] failed to send insert reply, result: %d\n", nresult);
    }
}

void CHostList::doUpdatePacket(CSocketRef* pSocket, CVepContainer* pContainer)
{
    dec_ptr<CVepContainer>  replyContainerPtr;
    center_packet_result_t  result;
    union centerPacketArgs* args;
    result_t                nresult;

    args = (union centerPacketArgs*)pContainer->getPacketHead();
    nresult = updateHost(args->update.id, args->update.ipAddr, args->update.strName);

    result.nresult = nresult;

    replyContainerPtr = new CVepContainer(VEP_CONTAINER_CENTER);
    replyContainerPtr->insertPacket(CENTER_PACKET_RESULT, &result, sizeof(result));

    nresult = m_pParent->getNetConnector()->send(replyContainerPtr, pSocket);
    if ( nresult != ESUCCESS )  {
        log_error(L_GEN, "[hostlist] failed to send update reply, result: %d\n", nresult);
    }
}

void CHostList::doDeletePacket(CSocketRef* pSocket, CVepContainer* pContainer)
{
    dec_ptr<CVepContainer>  replyContainerPtr;
    center_packet_result_t  result;
    union centerPacketArgs* args;
    result_t                nresult;

    args = (union centerPacketArgs*)pContainer->getPacketHead();
    nresult = deleteHost(args->remove.id);

    result.nresult = nresult;

    replyContainerPtr = new CVepContainer(VEP_CONTAINER_CENTER);
    replyContainerPtr->insertPacket(CENTER_PACKET_RESULT, &result, sizeof(result));

    nresult = m_pParent->getNetConnector()->send(replyContainerPtr, pSocket);
    if ( nresult != ESUCCESS )  {
        log_error(L_GEN, "[hostlist] failed to send remove reply, result: %d\n", nresult);
    }
}


/*
 * Process a VEP container
 *
 *      pSocket         connected client socket
 *      pContainer      valid container
 */
void CHostList::processPacket(CSocketRef* pSocket, CVepContainer* pContainer)
{
    vep_container_type_t    contType;

    shell_assert(pSocket);
    shell_assert(pSocket->isOpen());
    shell_assert(pContainer);
    shell_assert(pContainer->isValid());

    contType = pContainer->getType();
    if ( contType == VEP_CONTAINER_CENTER ) {
        CVepPacket*     pPacket = (*pContainer)[0];

        switch ( pPacket->getType() )  {
            case CENTER_PACKET_HEARTBEAT:
                doHeartBeatPacket(pSocket);
                break;

            case CENTER_PACKET_INSERT:
                doInsertPacket(pSocket, pContainer);
                break;

            case CENTER_PACKET_UPDATE:
                doUpdatePacket(pSocket, pContainer);
                break;

            case CENTER_PACKET_REMOVE:
                doDeletePacket(pSocket, pContainer);
                break;

            default:
                log_debug(L_GEN, "[heartbeat] unexpected packet, ignored\n");
                pPacket->dump(contType);
                break;
        }
    }
    else {
        log_debug(L_GEN, "[host_list] unsupported container type %u, ignored\n", contType);
    }
}

/*
 * Event processor
 *
 *      type        event type, EV_xxx
 */
boolean_t CHostList::processEvent(CEvent* pEvent)
{
    CEventTcpConnRecv*      pEventRecv;
    boolean_t               bProcessed = FALSE;

    switch ( pEvent->getType() )  {
        case EV_HEARTBEAT_REPLY:
            processHBResult((result_t)pEvent->getnParam());
            bProcessed = TRUE;
            break;

        case EV_NETCONN_RECV:
            pEventRecv = dynamic_cast<CEventTcpConnRecv*>(pEvent);
            shell_assert(pEventRecv);
            processPacket(pEventRecv->getSocket(), dynamic_cast<CVepContainer*>(pEventRecv->getContainer()));
            bProcessed = TRUE;
            break;
    }

    return bProcessed;
}

/*
 * HeartBeat timer
 */

void CHostList::stopHBTimer()
{
    SAFE_DELETE_TIMER(m_pHBTimer, m_pParent->getMainLoop());
}

void CHostList::startHBTimer()
{
    shell_assert(!m_pHBTimer);
    m_pHBTimer = new CTimer(HOST_PING_INTERVAL, TIMER_CALLBACK(CHostList::hbTimerHandler, this),
                        CTimer::timerPeriodic, "HeartBeat");
    m_pParent->getMainLoop()->insertTimer(m_pHBTimer);
}

void CHostList::hbTimerHandler(void* p)
{
    if ( !m_bHBPending )  {
        doHeartBeat();
    }
}

/*
 * Start module
 *
 * Return: ESUCCESS, ...
 */
result_t CHostList::init()
{
    CDbSql*         dbSql;
    CSqlResult*     pSqlResult;
    char            strTmp[128];
    CString         strPath;
    result_t        nresult, nr;

    /*
     * Initialise storage
     */
    strPath = m_pParent->getAppPath();
	strPath.appendPath("/data");
    CFile::validatePath(strPath);
    strPath.appendPath("center.db");

    dbSql = new CDbSqlite(strPath);
    nresult = dbSql->connect();
    if ( nresult != ESUCCESS )  {
        log_error(L_GEN, "[hostlist] can't connect to database %s, result: %d\n",
                  (const char*)strPath, nresult);
        delete dbSql;
        return nresult;
    }

    /*
     * Load host list
     */
    m_pStoreTable = new CHostTable(dbSql);
    m_pStoreTable->createTable();

    snprintf(strTmp, sizeof(strTmp), "SELECT id, ip, name FROM %s ORDER BY id", SPEC_TABLE_NAME);
    nr = m_pStoreTable->iterateStart(strTmp, &pSqlResult);
    while ( nr == ESUCCESS )  {
        if ( pSqlResult->getComumns() == 3 )  {
            host_id_t       id;
            ip_addr_t       ip;
            char            strTempName[HOSTLIST_NAME_MAX];

            id = (host_id_t)(*pSqlResult)[0].valInt;
            ip = (ip_addr_t)(*pSqlResult)[1].valInt;
            copyString(strTempName, (*pSqlResult)[2].valText, HOSTLIST_NAME_MAX);

            insertHostTable(id, ip, strTempName);
        }

        nr = m_pStoreTable->iterateNext(pSqlResult);
    }
    m_pStoreTable->iterateEnd(pSqlResult);

    dbSql->disconnect();

	dumpTable();

    m_bModified = TRUE;
    return ESUCCESS;
}

/*
 * Stop module
 */
void CHostList::terminate()
{
    stopHBTimer();
    SAFE_FREE(m_pHBTable);
    SAFE_DELETE(m_pStoreTable);
}

/*******************************************************************************
 * Debug support
 */

void CHostList::dump(const char* strPref) const
{
    size_t  i;

    log_dump("[hostlist] -- %sHeartBeat Table dump, count %d --\n",
             strPref ? strPref : "", m_nHBCount);

    for(i=0; i<m_nHBCount; i++)  {
        char    strTmp[32] = "FAILED";

        if ( m_pHBTable[i].hrAverage != ICMP_ITER_FAILED ) {
            if ( m_pHBTable[i].hrAverage > HR_1MSEC) {
                snprintf(strTmp, sizeof(strTmp), "%u ms",
                         (unsigned)HR_TIME_TO_MILLISECONDS(m_pHBTable[i].hrAverage));
            }
            else {
                snprintf(strTmp, sizeof(strTmp), "0.%u ms",
                         (unsigned)HR_TIME_TO_MICROSECONDS(m_pHBTable[i].hrAverage));
            }
        }

        log_dump(" | %2d: %-14s\t %s\n",
                 i, (const char*)CNetHost(m_pHBTable[i].dstAddr), strTmp);
    }
}

void CHostList::dumpTable(const char* strPref) const
{
    chostlist_iterator_t    it;
    size_t                  count = m_list.size();

    log_dump("-- %sHostList dump: %u host --\n", strPref ? strPref : "", count);
    log_dump("  id      ip                 name\n");
    log_dump("--------------------------------------\n");

    for(it=m_list.begin(); it != m_list.end(); it++) {
        log_dump("  %-3u     %-14s     %s\n",
                 it->second->id,
                 (const char*)CNetHost(it->second->ipAddr),
                 it->second->strName);
    }

    log_dump("-------------------------------------\n");
}
