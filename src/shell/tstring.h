/*
 *  Shell library
 *  Independent string operations
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 10.01.2017, 10:56:54
 *      Initial revision.
 */

#ifndef __SHELL_TSTRING_H_INCLUDED__
#define __SHELL_TSTRING_H_INCLUDED__

#include <string.h>

#define _tstrlen        strlen
#define _tstrcpy        strcpy
#define _tstrcat        strcat
#define _tstrcmp        strcmp
#define _tstrchr        strchr
#define _tstrrchr       strrchr
#define _tstrstr        strstr
#define _tstrspn        strspn
#define _tstrcspn       strcspn
#define _tstrcasecmp    strcasecmp
#define _tstrncasecmp   strncasecmp

#define _tmemset        memset
#define _tmemcmp        memcmp
#define _tmemmove       memmove
#define _tmemcpy        memcpy
#define _tmemmem        memmem
#define _tmemchr        memchr
#define _tmemrchr       memrchr

#define _tbzero(__p, __length)        _tmemset((__p), 0, (__length))
#define _tbzero_object(__obj)        _tbzero(&(__obj), sizeof(__obj))

extern void copyString(char* strDst, const char* strSrc, size_t nDstLen);
extern void copySubstring(char* strDst, const void* memSrc, size_t nSrcLen, size_t nDstLen);
extern void appendPath(char* strPath, const char* strSubPath, size_t nMaxLen);

extern void rTrim(char* strSrc, const char* chars);
extern void lTrim(char* strSrc, const char* chars);

#define rTrimEol(__s)    rTrim(__s, "\r\n")
#define rTrimWs(__s)    rTrim(__s, " \t")

#define SKIP_CHARS(__s, __chars)    \
    do {    \
        while( *(__s) != '\0' && _tstrchr((__chars), *(__s)) != NULL )  (__s)++; \
    } while(0)

#define MSKIP_CHARS(__s, __len, __chars)	\
	do { \
		while ( (__len) > 0 && _tstrchr((__chars), *(__s)) != NULL ) { \
			(__s)++; (__len)--; \
		} \
	} while (0)


#define SKIP_NON_CHARS(__s, __chars)    \
    do {    \
        while( *(__s) != '\0' && _tstrchr((__chars), *(__s)) == NULL )  (__s)++; \
    } while(0)

#endif /* __SHELL_TSTRING_H_INCLUDED__ */
