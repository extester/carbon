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
 *  Revision 1.0, 20.05.2013 11:32:04
 *      Initial revision.
 */

#include <stdio.h>
#include <iconv.h>
#include <unistd.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/utils.h"


/*
 * Copy string
 *
 *      strDst          destination buffer
 *      strSrc          source string
 *      nDstLen         maximum destination buffer size
 */
void copyString(char* strDst, const char* strSrc, size_t nDstLen)
{
    size_t	l = _tstrlen(strSrc);

    l = sh_min(l, (nDstLen-1));

    UNALIGNED_MEMCPY(strDst, strSrc, l);
    strDst[l] = '\0';
}

/*
 * Copy substring to the destination string
 *
 *      strDst          destination buffer
 *      memSrc          substring address
 *      nSrcLen         substring length
 *      nDstLen         maximum destination buffer size
 */
void copySubstring(char* strDst, const void* memSrc, size_t nSrcLen, size_t nDstLen)
{
    size_t  l = sh_min(nSrcLen, (nDstLen-1));

    shell_assert(nDstLen != 0);

    UNALIGNED_MEMCPY(strDst, memSrc, l);
    strDst[l] = '\0';
}

/*
 * Append sub-path to the path
 *
 *      strPath         path to append to
 *      strSubPath      path to add
 *      nMaxLen         maximum path size (including size of the 'strPath')
 */
void appendPath(char* strPath, const char* strSubPath, size_t nMaxLen)
{
	size_t	l = strlen(strPath);

    if ( l < (nMaxLen-1) ) {
        if ( strPath[l-1] != '/' ) {
            strPath[l] = '/';
            strPath[l+1] = '\0';
            l++;
        }

        copyString(&strPath[l], *strSubPath == '/' ? strSubPath + 1 : strSubPath,
                   (nMaxLen - l));
    }
}

/*
 * Remove trailing characters from the string
 *
 *      strSrc      existing string
 *      chars       characters to remove
 */
void rTrim(char* strSrc, const char* chars)
{
    size_t  length = _tstrlen(strSrc);
    char*   p;

    for ( p=strSrc+length-1; p>=strSrc; p-- )  {
        if ( _tstrchr(chars, *p) != NULL ) {
            *p = '\0';
        }
        else {
            break;
        }
    }
}

/*
 * Remove staring characters from the string
 *
 *      strSrc      existing string
 *      char        characters to remove
 */
void lTrim(char* strSrc, const char* chars)
{
	char*	s = strSrc;
	size_t 	len;

    while ( *s != '\0' && _tstrchr(chars, *s) != 0  )  {
    	s++;
    }

    if ( s != strSrc )  {
    	len = _tstrlen(s);
    	_tmemmove(strSrc, s, len+1);
    }
}


static uint16_t _crc16(uint16_t crc, uint8_t s)
{
    int i;

    crc ^= s;

    for (i = 0; i < 8; ++i) {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc = (crc >> 1);
    }

    return crc;
}

/*
 * Calculate a 16-bit CRC
 *
 *      pData       data pointer
 *      nSize       data size, bytes
 *
 * Return: crc16
 */
uint16_t crc16(const void* pData, size_t nSize)
{
    size_t          i;
    uint16_t        res = 0xFFFF;
    const uint8_t*  p = (const uint8_t*)pData;

    for (i = 0; i < nSize; i++)  {
        res = _crc16(res, p[i]);
    }

    return res;
}

/*
 * Parse version string "M.N" from string representation
 *
 *      strVer          version string
 *      pVersion        binary version [out]
 *
 * Return: ESUCCESS, EINVAL
 */
result_t parseVersion(const char* strVer, version_t* pVersion)
{
    int         major, minor, n;
    result_t    nresult = EINVAL;

    n = _tsscanf(strVer, "%d.%d", &major, &minor);
    if ( n == 2 )  {
        *pVersion = MAKE_VERSION(major, minor);
        nresult = ESUCCESS;
    }

    return nresult;
}

/*
 * Convert text macaddr to binary
 *
 *      strMac      text representation
 *      pMac        binary representation, length of ETHER_ADDR_LEN [out]
 *
 * Return: count of parsed numbers (must be ETHER_ADDR_LEN)
 */
int parseMacAddr(const char* strMac, uint8_t* pMac)
{
    const char* s;
	char*		endp;
    uint32_t    xx;
    int         i;

    i = 0;
    s = strMac;

    while ( i < ETHER_ADDR_LEN )  {
        SKIP_CHARS(s, " \t:");
        xx = _tstrtoul(s, &endp, 16);
        if ( *endp == ':' || *endp == ' ' || *endp == '\t' ||
             (*endp == '\0' && i == (ETHER_ADDR_LEN-1)) )
        {
            if ( xx > 255 )  {
                /* Number overflow */
                break;
            }

			pMac[i] = xx&0xff;
            i++;
        }
        else {
            /* Invalid hexadecimal number */
            break;
        }

        s = endp;
    }

    return i;
}

/*
 * Convert binary macaddr to text
 *
 *      pMac        binary representation
 *      strMac      text representation [out]
 */
void convMacAddr(const uint8_t* pMac, char* strMac)
{
    sprintf(strMac, "%02x:%02x:%02x:%02x:%02x:%02x",
                pMac[0]&0xff, pMac[1]&0xff, pMac[2]&0xff,
                pMac[3]&0xff, pMac[4]&0xff, pMac[5]&0xff);
}
