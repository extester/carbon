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
 *  Revision 1.0, 02.06.2015 12:36:46
 *      Initial revision.
 */

#ifndef __PACKET_CENTER_H_INCLUDED__
#define __PACKET_CENTER_H_INCLUDED__

#include "carbon/carbon.h"
#include "vep/vep.h"

#define VEP_CONTAINER_CENTER    2

const vep_packet_type_t CENTER_PACKET_NONE                  = 0;
const vep_packet_type_t CENTER_PACKET_RESULT                = 1;
const vep_packet_type_t CENTER_PACKET_HEARTBEAT             = 2;
const vep_packet_type_t CENTER_PACKET_HEARTBEAT_REPLY       = 3;
const vep_packet_type_t CENTER_PACKET_INSERT                = 4;
const vep_packet_type_t CENTER_PACKET_INSERT_REPLY          = 5;
const vep_packet_type_t CENTER_PACKET_UPDATE                = 6;
const vep_packet_type_t CENTER_PACKET_REMOVE                = 7;

typedef uint32_t            db_id_t;
typedef db_id_t             host_id_t;

#define HOSTLIST_NAME_MAX       32


typedef struct
{
    host_id_t       id;
    ip_addr_t       ipAddr;                     /* Host IP address */
    char            strName[HOSTLIST_NAME_MAX]; /* Host name */
    hr_time_t       hrPing;                     /* Host ping or ICMP_ITER_FAILED/ICMP_ITER_NOTSET */
} __attribute__ ((packed)) heartbeat_t;

/*******************************************************************************/

/*
 * CENTER_PACKET_RESULT
 */
typedef struct
{
    vep_packet_head_t       h;
    result_t                nresult;
} __attribute__ ((packed)) center_packet_result_t;


/*
 * CENTER_PACKET_HEARTBEAT, CENTER_PACKET_HEARTBEAT_REPLY
 */
typedef struct
{
    vep_packet_head_t       h;
} __attribute__ ((packed)) center_packet_heartbeat_t;

typedef struct
{
    vep_packet_head_t       h;
    heartbeat_t             list[];
} __attribute__ ((packed)) center_packet_heartbeat_reply_t;


/*
 * CENTER_PACKET_INSERT, CENTER_PACKET_INSERT_REPLY
 */
typedef struct
{
    vep_packet_head_t       h;
    ip_addr_t               ipAddr;
    char                    strName[HOSTLIST_NAME_MAX];
} __attribute__ ((packed)) center_packet_insert_t;

typedef struct
{
    vep_packet_head_t       h;
    result_t                nresult;
    host_id_t               id;
} __attribute__ ((packed)) center_packet_insert_reply_t;

/*
 * CENTER_PACKET_UPDATE
 */
typedef struct
{
    vep_packet_head_t       h;
    host_id_t               id;
    ip_addr_t               ipAddr;
    char                    strName[HOSTLIST_NAME_MAX];
} __attribute__ ((packed)) center_packet_update_t;

/*
 * CENTER_PACKET_REMOVE
 */
typedef struct
{
    vep_packet_head_t       h;
    host_id_t               id;
} __attribute__ ((packed)) center_packet_remove_t;



/*******************************************************************************
 * All supported CENTER packets
 */
union centerPacketArgs
{
    vep_packet_head_t                   h;

    center_packet_result_t              result;

    center_packet_heartbeat_t           heartbeat;
    center_packet_heartbeat_reply_t     heartbeat_reply;

    center_packet_insert_t              insert;
    center_packet_insert_reply_t        insert_reply;

    center_packet_update_t              update;
    center_packet_remove_t              remove;
};

extern void initCenterPacket();

#endif  /* __PACKET_CENTER_H_INCLUDED__ */
