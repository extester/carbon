/*
 *  Carbon framework
 *  Carbon events
 *
 *  Copyright (c) 2015-2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 02.06.2015 10:59:23
 *      Initial revision.
 *
 *  Revision 1.1, 16.01.2017 12:46:04
 *  	Added events: EV_QUIT, EV_HUP, EV_USR1, EV_USR2
 */

#ifndef __CARBON_EVENT_H_INCLUDED__
#define __CARBON_EVENT_H_INCLUDED__

#include "shell/config.h"

#include "carbon/event/event.h"
#if CARBON_DEBUG_EVENT
#include "carbon/event/event_debug.h"
#endif /* CARBON_DEBUG_EVENT */

/*
 * List of all available events
 */

/*
 * Common packets
 */
#define EV_UNDEFINED						0
#define EV_DUMP								1
#define EV_DEBUG							2
#define EV_START							3

#define EV_QUIT								4
#define EV_HUP								5
#define EV_USR1								6
#define EV_USR2								7

/*
 * Network Connector (CNetConnector) events
 */
#define EV_NETCONN_DO_CONNECT				8		/* Internal network connector packets */
#define EV_NETCONN_DO_SEND					9		/* Sending packets */
#define EV_NETCONN_DO_SEND_LOCAL			10
#define EV_NETCONN_DO_SENDTO				11
#define EV_NETCONN_DO_RECVFROM				12
#define EV_NETCONN_DO_IO					13		/* Send/Receive packets */
#define EV_NETCONN_RECV						14		/* Received packets */
#define EV_NETCONN_RECVFROM					15		/* Received packets */
#define EV_NETCONN_SENT						16		/* Operation result, pparam=sessId, nparam=nresult */

/*
 * Net Server
 */
#define EV_NET_SERVER_CONNECTED				17

#define EV_NET_CLIENT_DO_CONNECT			18
#define EV_NET_CLIENT_DO_DISCONNECT			19
#define EV_NET_CLIENT_CONNECTED				20
#define EV_NET_CLIENT_DISCONNECTED			21
#define EV_NET_CLIENT_DO_SEND				22
#define EV_NET_CLIENT_SENT					23

#define EV_NET_SERVER_DO_RECV				24
#define EV_NET_SERVER_RECV					25

#define EV_NET_CLIENT_DO_RECV				26
#define EV_NET_CLIENT_RECV					27

#define EV_NET_SERVER_DO_SEND				28
#define EV_NET_SERVER_SENT					29

#define EV_NET_CLIENT_DO_IO					30
#define EV_NET_SERVER_DISCONNECTED			31

#define EV_R_SHELL_EXECUTE					32
#define EV_R_SHELL_EXECUTE_REPLY			33

#define EV_DNS_RESOLVE						34
#define EV_DNS_REPLY						35

#define EV_NTP_CLIENT_REQUEST				36
#define EV_NTP_CLIENT_REPLY					37

/* First user-defined events */
#define EV_USER								(EV_NTP_CLIENT_REPLY+1)

extern void registerCarbonEventString();


#endif /* __CARBON_EVENT_H_INCLUDED__ */
