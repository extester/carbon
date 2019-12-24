/*
 *  Shell library
 *	Debugging utilities
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 28.06.2013 13:30:11
 *      Initial revision.
 */

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/debug.h"


/*
 * Hexadecimal dump
 *
 *      pData			data address
 *      nLength			data length, bytes
 */
void dumpHex(const void* pData, size_t nLength)
{
    const uint8_t*  p = (const uint8_t*)pData;
    size_t          i;

    nLength = MIN(nLength, 100);
    for(i=0; i<nLength; i++)  {
        printf("%02X ", p[i]&0xff);
    }

    printf("\n");
    fflush(stdout);
}

/*
 * Hexadecimal dump to string
 *
 *      strBuf      	output string
 *      nBufLength		output string max. length, bytes
 *      pData       	data address
 *      nLength			data length, bytes
 */
void getDumpHex(char* strBuf, size_t nBufLength, const void* pData, size_t nLength)
{
    const uint8_t*  p = (const uint8_t*)pData;
    size_t          i, l = 0;

    for(i=0; i<nLength; i++) {
        if ( (l+3) < nBufLength )  {
            l += _tsnprintf(&strBuf[l], nBufLength-l, "%02X ", p[i]&0xff);
        }
        else {
            break;
        }
    }
}
