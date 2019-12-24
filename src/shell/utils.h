/*
 *  Shell library
 *  Utility functions
 *
 *  Copyright (c) 2013 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 20.05.2013 11:33:12
 *      Initial revision.
 */

#ifndef __SHELL_UTILS_H_INCLUDED__
#define __SHELL_UTILS_H_INCLUDED__

#include <net/ethernet.h>

#include "shell/types.h"

extern result_t convertEncoding(const char* incode, const char* outcode,
		 const char* inbuf, size_t insize, char* outbuf, size_t* poutsize);

extern uint16_t crc16(const void* pData, size_t nSize);
extern void sleep_s(unsigned int nSecond);
extern void sleep_ms(unsigned int nMillisecond);
extern void sleep_us(unsigned int nMicrosecond);
extern result_t parseVersion(const char* strVer, version_t* pVersion);

extern int parseMacAddr(const char* strMac, uint8_t* pMac);
extern void convMacAddr(const uint8_t* pMac, char* strMac);


#if __BYTE_ORDER == __LITTLE_ENDIAN
static inline uint64_t ntohll(uint64_t value)
{
	union {
		uint64_t value64;
		struct { uint8_t x1; uint8_t x2; uint8_t x3; uint8_t x4;
			uint8_t x5; uint8_t x6; uint8_t x7; uint8_t x8; } x;
	} x, y;

	x.value64 = value;

	y.x.x1 = x.x.x8;
	y.x.x2 = x.x.x7;
	y.x.x3 = x.x.x6;
	y.x.x4 = x.x.x5;
	y.x.x5 = x.x.x4;
	y.x.x6 = x.x.x3;
	y.x.x7 = x.x.x2;
	y.x.x8 = x.x.x1;

	return y.value64;
}

static inline uint64_t htonll(uint64_t value)
{
	return ntohll(value);
}
#else
#define ntohll(x)	(x)
#define htonll(x)	(x)
#endif

#endif /* __SHELL_UTILS_H_INCLUDED__ */
