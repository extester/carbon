/*
 *  Carbon framework
 *	Object Tracer utility
 *
 *  Copyright (c) 2013 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 28.06.2013 16:26:57
 *      Initial revision.
 */

#include "carbon/logger.h"
#include "carbon/object_tracer.h"


#if DEBUG

/*******************************************************************************
 * CObjectTracer
 */

void CObjectTracer::insert(const char* name, const void* p)
{
    object_trace_t  obj;

    obj.name = name;
    obj.p = p;

    m_arObject.lock();
    m_arObject.insert_last(obj);
    m_arObject.unlock();
}

void CObjectTracer::remove(const void* p)
{
    CAutoLock   locker(m_arObject);
    RLockedList<object_trace_t>::iterator iter = m_arObject.begin();

    while ( iter != m_arObject.end() )  {
        if ( (*iter).p == p )  {
            iter = m_arObject.erase(iter);
        }
        else  {
            iter++;
        }
    }
}

void CObjectTracer::print()
{
    CAutoLock   locker(m_arObject);
    RLockedList<object_trace_t>::iterator iter = m_arObject.begin();

    if ( m_arObject.is_empty() )  {
        /*log_dump("*** ObjectTracer contains NO objects ***\n");*/
    }
    else  {
    	log_dump("ObjectTracer (%d objects):\n", m_arObject.count());

        while ( iter != m_arObject.end() )  {
        	log_dump("   -- %s \t(%p)\n", (const char*)(*iter).name, ((*iter).p));
            iter++;
        }
    }
}

CObjectTracer* g_objectTracer = 0;

void initObjectTracer()
{
    shell_assert(g_objectTracer == 0);
    g_objectTracer = new CObjectTracer();
}

void exitObjectTracer()
{
	shell_assert(g_objectTracer != 0);
    SAFE_DELETE(g_objectTracer);
}

void insertObjectTracer(const char* name, const void* p)
{
	shell_assert(g_objectTracer != 0);
    g_objectTracer->insert(name, p);
}

void insertObjectTracer(const void* p)
{
	shell_assert(g_objectTracer != 0);
    g_objectTracer->insert(p);
}

void removeObjectTracer(const void* p)
{
	shell_assert(g_objectTracer != 0);
    g_objectTracer->remove(p);
}

void printObjectTracer()
{
	shell_assert(g_objectTracer != 0);
    g_objectTracer->print();
}

#endif /* DEBUG */
