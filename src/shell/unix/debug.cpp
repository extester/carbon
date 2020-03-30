/*
 *  Shell library
 *	Debugging utilities (UNIX)
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

#include <stdio.h>
#include <malloc.h>
#include <sys/sysinfo.h>
#include <execinfo.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/debug.h"

#define BACKTRACE_FRAMES		24


/*
 *  Print stack backtrace
 */
void printBacktrace()
{
    void*   addrList[BACKTRACE_FRAMES+1];
    char**  symList;
    int     length, i;

    length = backtrace(addrList, ARRAY_SIZE(addrList));
    if ( length == 0 )  {
    	log_dump("  -- no backtrace available --\n");
        return;
    }

    symList = backtrace_symbols(addrList, length);
    for(i=0; i<length; i++)  {
    	log_dump("  -- %s\n", symList[i]);
    }

    free(symList);
}

/*
 * Get free and full memory size, Kbytes
 *
 *      pFullMemory     full memory size, Kb (can be NULL) [out]
 *
 * Return: free memory size, Kb
 */
int getFreeMemory(int* pFullMemory)
{
    struct sysinfo  si;
    int             ret, i;
    FILE*           fp;
    char            buf[128], *p;
    int             free_size, buf_size, cache_size;

    if ( pFullMemory )  {
        *pFullMemory = 0;
    }

    ret = sysinfo(&si);
    if ( ret != 0 )  {
        return 0;
    }

    fp = fopen("/proc/meminfo", "r");
    if ( !fp )  {
        return 0;
    }

    for(i = 0; i <= 3; i++) {
        char*   s;

        s = fgets(buf, 127, fp);
		shell_unused(s);
    }
    fclose(fp);

    p = buf + _tstrcspn(buf, "0123456789");
    cache_size = _tatoi(p);

    free_size = (int)( (uint64_t)si.mem_unit*si.freeram/1024);
    buf_size = (int)( (uint64_t)si.mem_unit*si.bufferram/1024);

    if ( pFullMemory )  {
        *pFullMemory = (int)((uint64_t)si.mem_unit*si.totalram/1024);
    }

    return free_size+buf_size+cache_size;
}
