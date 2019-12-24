/*
 *  Carbon/Vep module
 *  System VEP packet string table
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.07.2015 12:44:37
 *      Initial revision.
 */

#include "vep/packet_system.h"

static const char* strPacketTable[] = {
    "NONE",
    "RESULT",
    "VERSION",
    "VERSION_REPLY",
    "MEMORY_STAT",
    "MEMORY_STAT_REPLY",
    "NETCONN_STAT",
    "NETCONN_STAT_REPLY",
    "GET_LOGGER_CHANNEL",
    "LOGGER_CHANNEL",
    "LOGGER_ENABLE",
    "GET_LOGGER_BLOCK",
    "LOGGER_BLOCK"
};

void initSystemPacket()
{
    CVepContainer::registerContainerName(VEP_CONTAINER_SYSTEM, "SYSTEM");
    CVepContainer::registerNames(VEP_CONTAINER_SYSTEM,
        SYSTEM_PACKET_NONE, SYSTEM_PACKET_NONE+ARRAY_SIZE(strPacketTable)-1,
        strPacketTable);
}
