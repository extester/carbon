/*
 *  Carbon framework
 *  Event debugging support
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 16.02.2017 11:10:30
 *      Initial revision.
 */

#ifndef __CARBON_EVENT_DEBUG_H_INCLUDED__
#define __CARBON_EVENT_DEBUG_H_INCLUDED__

#include <list>

#include "shell/config.h"

#include "carbon/lock.h"
#include "carbon/event.h"

typedef struct
{
    event_type_t        nFirst;         /* First event number */
    event_type_t        nLast;          /* Last event number (included) */
    const char**        strTable;       /* String table */
} event_string_table_t;


class CEventStringTable
{
    protected:
        static std::list<event_string_table_t>  m_strTable;
        static CMutex                           m_strTableLock;

    public:
        static result_t registerStringTable(event_type_t nFirst, event_type_t nLast,
                                            const char** strTable);
        static void dumpStringTable(const char* strPref = "");

    public:
        static void getEventName(const CEvent* pEvent, char* strBuffer, size_t length);
};

#endif /* __CARBON_EVENT_DEBUG_H_INCLUDED__ */
