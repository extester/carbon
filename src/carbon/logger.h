/*
 *  Carbon framework
 *  Carbon specific logger definitions
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.09.2015 13:33:43
 *      Initial revision.
 *
 *  Revision 1.1, 25.12.2019 20:30:50
 *  	Removed all L_*_FL log levels and changed to
 *  	log_debug(L_*_FL, ..) => log_trace(L_*, ..)
 */

#ifndef __CARBON_LOGGER_H_INCLUDED__
#define __CARBON_LOGGER_H_INCLUDED__

#include "shell/logger.h"

#define L_NETCONN					(L_SHELL_USER+0)
#define L_NETCONN_IO				(L_SHELL_USER+1)

#define L_NETSERV					(L_SHELL_USER+2)
#define L_NETSERV_IO				(L_SHELL_USER+3)

#define L_NETCLI					(L_SHELL_USER+4)
#define L_NETCLI_IO					(L_SHELL_USER+5)

#define L_SHELL_EXECUTE				(L_SHELL_USER+6)
#define L_HTTP_CONT					(L_SHELL_USER+7)

/* NetMedia library */
#define L_NET_MEDIA					(L_SHELL_USER+8)
#define L_RTSP						(L_SHELL_USER+9)
#define L_SDP						(L_SHELL_USER+10)
#define L_RTP						(L_SHELL_USER+11)
#define L_RTCP						(L_SHELL_USER+12)

#define	L_RTP_VIDEO_H264			(L_SHELL_USER+13)
#define L_MP4FILE					(L_SHELL_USER+14)
#define L_MP4FILE_DYN				(L_SHELL_USER+15)

//#define L_MEDIA						(L_SHELL_USER+16)

/* Contact library */
#define L_MODEM						(L_SHELL_USER+16)
#define L_MODEM_IO					(L_SHELL_USER+17)

#define L_NET_ROUTE					(L_SHELL_USER+18)
#define L_DNS_CLIENT				(L_SHELL_USER+19)

#define L_MAIL						(L_SHELL_USER+20)

#define L_SQL						(L_SHELL_USER+21)

#define L_CARBON_USER				(L_SQL+1)

#endif /* __CARBON_LOGGER_H_INCLUDED__ */
