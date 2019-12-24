/*
 *  Shell library
 *  Network utility functions
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 06.05.2015 23:22:06
 *      Initial revision.
 */

#ifndef __SHELL_NETUTILS_H_INCLUDED__
#define __SHELL_NETUTILS_H_INCLUDED__

#include <netinet/in.h>

#include "shell/types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern result_t getInterfaceIp(const char* strIface, struct in_addr* pInaddr);
extern uint16_t in_cksum(void* addr, int len);


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __SHELL_NETUTILS_H_INCLUDED__ */
