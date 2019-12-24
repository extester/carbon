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
 *  Revision 1.0, 22.07.2015 18:54:11
 *      Initial revision.
 */

#ifndef __CARBON_PACKET_SYSTEM_H_INCLUDED__
#define __CARBON_PACKET_SYSTEM_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/memory.h"
#include "carbon/net_connector/tcp_connector.h"
#include "vep/vep.h"

const vep_packet_type_t SYSTEM_PACKET_NONE                  = 0;
const vep_packet_type_t SYSTEM_PACKET_RESULT                = 1;
const vep_packet_type_t SYSTEM_PACKET_VERSION               = 2;
const vep_packet_type_t SYSTEM_PACKET_VERSION_REPLY         = 3;
const vep_packet_type_t SYSTEM_PACKET_MEMORY_STAT           = 4;
const vep_packet_type_t SYSTEM_PACKET_MEMORY_STAT_REPLY     = 5;
const vep_packet_type_t SYSTEM_PACKET_NETCONN_STAT          = 6;
const vep_packet_type_t SYSTEM_PACKET_NETCONN_STAT_REPLY    = 7;
const vep_packet_type_t SYSTEM_PACKET_GET_LOGGER_CHANNEL    = 8;
const vep_packet_type_t SYSTEM_PACKET_LOGGER_CHANNEL        = 9;
const vep_packet_type_t SYSTEM_PACKET_LOGGER_ENABLE         = 10;
const vep_packet_type_t SYSTEM_PACKET_GET_LOGGER_BLOCK      = 11;
const vep_packet_type_t SYSTEM_PACKET_LOGGER_BLOCK          = 12;


/*******************************************************************************/

/*
 * SYSTEM_PACKET_RESULT
 */
typedef struct
{
	vep_packet_head_t   h;
    result_t            nresult;
} __attribute__ ((packed)) system_packet_result_t;

/*
 * SYSTEM_PACKET_VERSION, CENTER_PACKET_VERSION_REPLY
 */

typedef struct
{
	vep_packet_head_t   h;
} __attribute__ ((packed)) system_packet_version_t;

typedef struct
{
	vep_packet_head_t   h;
    version_t           version;
    version_t           lib_version;
} __attribute__ ((packed)) system_packet_version_reply_t;

/*
 * SYSTEM_PACKET_MEMORY_STAT, SYSTEM_PACKET_MEMORY_STAT_REPLY
 */
typedef struct
{
	vep_packet_head_t   h;
} __attribute__ ((packed)) system_packet_memory_stat_t;

typedef struct
{
    vep_packet_type_t   h;
    memory_stat_t       stat;
} __attribute__ ((packed)) system_packet_memory_stat_reply_t;

/*
 * SYSTEM_PACKET_NETCONN_STAT, SYSTEM_PACKET_NETCONN_STAT_REPLY
 */
typedef struct
{
	vep_packet_head_t   h;
} __attribute__ ((packed)) system_packet_netconn_stat_t;

typedef struct
{
	vep_packet_head_t   h;
    tcpconn_stat_t      stat;
} __attribute__ ((packed)) system_packet_netconn_stat_reply_t;

/*
 * SYSTEM_PACKET_GET_LOGGER_CHANNEL, SYSTEM_PACKET_LOGGER_CHANNEL
 */
typedef struct
{
	vep_packet_head_t   h;
} __attribute__ ((packed)) system_packet_get_logger_channel_t;

typedef struct
{
	vep_packet_head_t   h;
    uint8_t             channel[LOGGER_CHANNEL_MAX/8];
} __attribute__ ((packed)) system_packet_logger_channel_t;

/*
 * SYSTEM_PACKET_LOGGER_ENABLE
 */
typedef struct
{
	vep_packet_head_t   h;
    uint32_t            levChan;
    uint16_t            enable;
} __attribute__ ((packed)) system_packet_logger_enable_t;

/*
 * SYSTEM_PACKET_GET_LOGGER_BLOCK, SYSTEM_PACKET_LOGGER_BLOCK
 */
typedef struct
{
	vep_packet_head_t   h;
    uint32_t            size;
} __attribute__ ((packed)) system_packet_get_logger_block_t;

typedef struct
{
	vep_packet_head_t   h;
    uint32_t            size;
    uint8_t             data[];
} __attribute__ ((packed)) system_packet_logger_block_t;


/*******************************************************************************
 * All supported SYSTEM packets
 */
union systemPacketArgs
{
	vep_packet_head_t   		        h;

    system_packet_result_t              result;

    system_packet_version_t             version;
    system_packet_version_reply_t       version_reply;

    system_packet_memory_stat_t         memory_stat;
    system_packet_memory_stat_reply_t   memory_stat_reply;

    system_packet_netconn_stat_t        netconn_stat;
    system_packet_netconn_stat_reply_t  netconn_stat_reply;

    system_packet_get_logger_channel_t  get_logger_channel;
    system_packet_logger_channel_t      logger_channel;

    system_packet_logger_enable_t       logger_enable;

    system_packet_get_logger_block_t    get_logger_block;
    system_packet_logger_block_t        logger_block;
};

extern void initSystemPacket();

#endif  /* __CARBON_PACKET_SYSTEM_H_INCLUDED__ */
