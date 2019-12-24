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
 *  Revision 1.0, 06.05.2015 23:23:17
 *      Initial revision.
 */

#include <unistd.h>
#include <net/if.h>
#include "sys/ioctl.h"
#include <arpa/inet.h>

#include "shell/shell.h"
#include "shell/utils.h"
#include "shell/net/netutils.h"

/*
 * Get interface IP address
 *
 * 		strIface		interface name, i.e. "eth0"
 * 		pInaddr			interface IP address [out]
 *
 * Return: ESYCCESS, ...
 */
result_t getInterfaceIp(const char* strIface, struct in_addr* pInaddr)
{
	struct ifreq	ifr;
	int 			fd;
	int				retVal;
	result_t		nresult = ESUCCESS;

	shell_assert(strIface);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if ( fd < 0 )  {
		return errno;
	}

	ifr.ifr_addr.sa_family = AF_INET;
	copyString(ifr.ifr_name, strIface, IFNAMSIZ);

	retVal = ioctl(fd, SIOCGIFADDR, &ifr);
	if ( retVal < 0 )  {
		nresult = errno;
		::close(fd);
	    return nresult;
	}

	::close(fd);

	*pInaddr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	return nresult;
}

/*
 * in_cksum --
 * Checksum routine for Internet Protocol
 * family headers (C Version)
 */
uint16_t in_cksum(void* addr, int len)
{
	int 		sum = 0;
	uint16_t 	answer = 0;
	uint16_t*	w = (uint16_t*)addr;
	int 		nleft = len;

	shell_assert((len&1) == 0);

	/*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
	while ( nleft > 1 )
	{
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if ( nleft == 1 )
	{
		*(uint8_t*) (&answer) = *(uint8_t*) w;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);		/* add hi 16 to low 16 */
	sum += (sum >> 16);						/* add carry */
	answer = (uint16_t)~sum;							/* truncate to 16 bits */

	return answer;
}
