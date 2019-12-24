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
 *	Revision 1.0, 28.06.2013 12:30:03
 *      Initial revision.
 */

#ifndef __SHELL_DEBUG_H_INCLUDED__
#define __SHELL_DEBUG_H_INCLUDED__

#include "shell/types.h"

extern void printBacktrace();
extern int getFreeMemory(int* pFullMemory);
extern void dumpHex(const void* pData, size_t nLength);
extern void getDumpHex(char* strBuf, size_t nBufLength, const void* pData, size_t nLength);

#endif /* __SHELL_DEBUG_H_INCLUDED__ */
