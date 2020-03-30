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
 *  Revision 1.0, 02.06.2015 10:21:52
 *      Initial revision.
 */

#include "shell/logger/logger_base.h"

#include "carbon/event.h"
#include "carbon/net_connector/tcp_connector.h"
#include "vep/packet_system.h"
#include "vep/sys_responder.h"


/*******************************************************************************
 * CSysResponder class
 */

CSysResponder::CSysResponder(CApplication* pApp, CTcpConnector* pNetConnector) :
    CModule("SystemResponder"),
    m_pApp(pApp),
    m_pNetConnector(pNetConnector)
{
}

CSysResponder::~CSysResponder()
{
}

/*
 * Send reply packet of type SYSTEM_PACKET_VERSION_TYPE
 *
 *      pSocket     open socket
 *
 */
result_t CSysResponder::doPacketVersion(CSocketRef* pSocket)
{
    dec_ptr<CVepContainer>          containerPtr;
    system_packet_version_reply_t   reply;
    result_t                        nresult;

    shell_assert(pSocket);

    if ( !pSocket->isOpen() )  {
        log_debug(L_GEN, "[sys_resp] socket is not connected\n");
        return EINVAL;
    }

    containerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM);
    reply.lib_version = carbon_get_version();
    reply.version = m_pApp->getVersion();

    nresult = containerPtr->insertPacket(SYSTEM_PACKET_VERSION_REPLY, &reply, sizeof(reply));
    shell_assert(nresult == ESUCCESS);

    nresult = m_pNetConnector->send(containerPtr, pSocket);
    return nresult;
}

result_t CSysResponder::doPacketMemoryStat(CSocketRef* pSocket)
{
    dec_ptr<CVepContainer>  containerPtr;
    memory_stat_t           stat;
    CMemoryManager*         pMemoryManager;
    result_t                nresult;

    shell_assert(pSocket);

    if ( !pSocket->isOpen() )  {
        log_debug(L_GEN, "[sys_resp] socket is not connected\n");
        return EINVAL;
    }

    _tbzero_object(stat);
    pMemoryManager = appMemoryManager();
    if ( pMemoryManager )  {
        pMemoryManager->getStat(&stat, sizeof(stat));
    }

    containerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM);
    nresult = containerPtr->insertPacket(SYSTEM_PACKET_MEMORY_STAT_REPLY, &stat, sizeof(stat));
    shell_assert(nresult == ESUCCESS);

    nresult = m_pNetConnector->send(containerPtr, pSocket);
    return nresult;
}

result_t CSysResponder::doPacketNetConnStat(CSocketRef* pSocket)
{
    dec_ptr<CVepContainer>  containerPtr;
    tcpconn_stat_t          stat;
    result_t                nresult;

    shell_assert(pSocket);

    if ( !pSocket->isOpen() )  {
        log_debug(L_GEN, "[sys_resp] socket is not connected\n");
        return EINVAL;
    }

    _tbzero_object(stat);
    if ( m_pNetConnector )  {
        m_pNetConnector->getStat(&stat, sizeof(stat));
    }

    containerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM);
    nresult = containerPtr->insertPacket(SYSTEM_PACKET_NETCONN_STAT_REPLY, &stat, sizeof(stat));
    shell_assert(nresult == ESUCCESS);

    nresult = m_pNetConnector->send(containerPtr, pSocket);
    return nresult;
}

result_t CSysResponder::doPacketGetLoggerChannel(CSocketRef* pSocket)
{
    dec_ptr<CVepContainer>  containerPtr;
    uint8_t                 channel[LOGGER_CHANNEL_MAX/8];
    result_t                nresult;

    shell_assert(pSocket);

    if ( !pSocket->isOpen() )  {
        log_debug(L_GEN, "[sys_resp] socket is not connected\n");
        return EINVAL;
    }

    _tbzero_object(channel);
    logger_get_channel(I_DEBUG, &channel, sizeof(channel));

    containerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM);
    nresult = containerPtr->insertPacket(SYSTEM_PACKET_LOGGER_CHANNEL, &channel, sizeof(channel));
    shell_assert(nresult == ESUCCESS);

    nresult = m_pNetConnector->send(containerPtr, pSocket);
    return nresult;
}

result_t CSysResponder::doPacketLoggerEnable(CSocketRef* pSocket, CVepContainer* pContainer)
{
    dec_ptr<CVepContainer>  resContainerPtr;
    result_t                nresult, nr;
    union systemPacketArgs*	args;
    system_packet_result_t  result;

    shell_assert(pSocket);

    if ( !pSocket->isOpen() )  {
        log_debug(L_GEN, "[sys_resp] socket is not connected\n");
        return EINVAL;
    }

    args = (union systemPacketArgs*)pContainer->getPacketHead();

    if ( args->logger_enable.enable )  {
        logger_enable(args->logger_enable.levChan);
    }
    else {
        logger_disable(args->logger_enable.levChan);
    }
    nr = ESUCCESS;

    result.nresult = nr;
    resContainerPtr = new CVepContainer(VEP_CONTAINER_SYSTEM);
    nresult = resContainerPtr->insertPacket(SYSTEM_PACKET_RESULT, &result, sizeof(result));
    shell_assert(nresult == ESUCCESS);

    nresult = m_pNetConnector->send(resContainerPtr, pSocket);
    return nresult;
}


/*
 * Process info-container
 *
 *      pSocket         connected client socket
 *      pContainer      valid info-container
 *
 */
result_t CSysResponder::processPacket(CEventTcpConnRecv* pEventPacket)
{
    CSocketRef*             pSocket;
    CVepContainer*          pContainer;
    CVepPacket*             pPacket;
    vep_container_type_t    contType;
    vep_packet_type_t       packType;
    char                    stmp[64];
    result_t                nresult;

    shell_assert(pEventPacket);
    if ( pEventPacket->getResult() != ESUCCESS )  {
        log_debug(L_GEN, "[sys_resp] received packet containing result=%d, shouldn't happen\n",
                  pEventPacket->getResult());
        return EINVAL;
    }

    pSocket = pEventPacket->getSocket();
    pContainer = dynamic_cast<CVepContainer*>(pEventPacket->getContainer());

    if ( !pSocket || !pContainer )  {
        log_debug(L_GEN, "[sys_resp] wrong parameters\n");
        return EINVAL;
    }

    if ( !pContainer->isValid() )  {
        log_debug(L_GEN, "[sys_resp] invalid container\n");
        return EFAULT;
    }

    contType = pContainer->getType();
    if ( contType != VEP_CONTAINER_SYSTEM )  {
        CVepContainer::getContainerName(contType, stmp, sizeof(stmp));
        log_debug(L_GEN, "[sys_resp] wrong container type %s(%d), expected SYSTEM\n",
                  stmp, contType);
        return EINVAL;
    }

    pPacket = (*pContainer)[0];
    packType = pPacket->getType();

    switch ( packType )  {
        case SYSTEM_PACKET_VERSION:
            nresult = doPacketVersion(pSocket);
            break;

        case SYSTEM_PACKET_MEMORY_STAT:
            nresult = doPacketMemoryStat(pSocket);
            break;

        case SYSTEM_PACKET_NETCONN_STAT:
            nresult = doPacketNetConnStat(pSocket);
            break;

        case SYSTEM_PACKET_GET_LOGGER_CHANNEL:
            nresult = doPacketGetLoggerChannel(pSocket);
            break;

        case SYSTEM_PACKET_LOGGER_ENABLE:
            nresult = doPacketLoggerEnable(pSocket, pContainer);
            break;

        default:
            nresult = ENOENT;

            CVepContainer::getPacketName(contType, packType, stmp, sizeof(stmp));
            log_debug(L_GEN, "[sys_resp] unsuported SYSTEM packet %s(%d) ignored\n",
                      stmp, packType);
            break;
    }

    return nresult;
}

result_t CSysResponder::init()
{
    return ESUCCESS;
}

void CSysResponder::terminate()
{
}

/*******************************************************************************
 * Debugging support
 */

void CSysResponder::dump(const char* strPref) const
{
    shell_unused(strPref);
}
