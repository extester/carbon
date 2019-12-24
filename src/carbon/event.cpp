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
 *  Revision 1.1, 16.01.2017 12:45:33
 *  	Added events: EV_QUIT, EV_HUP, EV_USR1, EV_USR2
 */

#include "shell/config.h"

#if CARBON_DEBUG_EVENT
#include "carbon/event/event_debug.h"

static const char* carbonEventStringTable[] =
{
		"EV_UNDEFINED",
		"EV_DUMP",
		"EV_DEBUG",
		"EV_START",

		"EV_QUIT",
		"EV_HUP",
		"EV_USR1",
		"EV_USR2",

		"EV_NETCONN_DO_CONNECT",
		"EV_NETCONN_DO_SEND",
		"EV_NETCONN_DO_SEND_LOCAL",
		"EV_NETCONN_DO_SENDTO",
		"EV_NETCONN_DO_RECVFROM",
		"EV_NETCONN_DO_IO",
		"EV_NETCONN_RECV",
		"EV_NETCONN_RECVFROM",
		"EV_NETCONN_SENT",

		"EV_NET_SERVER_CONNECTED",
		"EV_NET_CLIENT_DO_CONNECT",
		"EV_NET_CLIENT_DO_DISCONNECT",
		"EV_NET_CLIENT_CONNECTED",
		"EV_NET_CLIENT_DISCONNECTED",
		"EV_NET_CLIENT_DO_SENT",
		"EV_NET_CLIENT_SENT",

		"EV_NET_SERVER_DO_RECV",
		"EV_NET_SERVER_RECV",
		"EV_NET_CLIENT_DO_RECV",
		"EV_NET_CLIENT_RECV",
		"EV_NET_SERVER_DO_SEND",
		"EV_NET_SERVER_SENT",

		"EV_NET_CLIENT_DO_IO",
		"EV_NET_SERVER_DISCONNECTED",

		"EV_R_SHELL_EXECUTE",
		"EV_R_SHELL_EXECUTE_REPLY",

		"EV_DNS_RESOLVE",
		"EV_DNS_REPLY",

		"EV_NTP_CLIENT_REQUEST",
		"EV_NTP_CLIENT_REPLY"
};

/*
 * Initialise event name strings
 */
void registerCarbonEventString()
{
	CEventStringTable::registerStringTable(EV_UNDEFINED, EV_USER-1, carbonEventStringTable);
}

#else /* CARBON_DEBUG_EVENT */

void registerCarbonEventString()
{}

#endif /* CARBON_DEBUG_EVENT */
