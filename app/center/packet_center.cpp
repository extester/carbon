/*
 *  Carbon Framework Network Center
 *  Center info-packets
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


#include "packet_center.h"

static const char* strPacketTable[] = {
    "NONE",
    "RESULT",
    "HEARTBEAT",
    "HEARTBEAT_REPLY",
    "INSERT",
    "INSERT_REPLY",
    "UPDATE",
    "REMOVE"
};

void initCenterPacket()
{
    CVepContainer::registerContainerName(VEP_CONTAINER_CENTER, "CENTER");
    CVepContainer::registerNames(VEP_CONTAINER_CENTER,
         CENTER_PACKET_NONE, CENTER_PACKET_REMOVE, strPacketTable);
}
