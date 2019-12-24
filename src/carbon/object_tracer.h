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
 *	Revision 1.0, 28.06.2013 12:23:27
 *      Initial revision.
 */

#ifndef __CARBON_OBJECT_TRACER_H_INCLUDED__
#define __CARBON_OBJECT_TRACER_H_INCLUDED__

#include "carbon/cstring.h"
#include "carbon/lock.h"

#if DEBUG

typedef struct
{
    CString			name;
    const void*     p;
} object_trace_t;

class CObjectTracer
{
    private:
        RLockedList<object_trace_t>     m_arObject;

    public:
        CObjectTracer() {}
        ~CObjectTracer() {}

    public:
        void insert(const char* name, const void* p);
        void insert(const void* p)  { insert("noname", p); }
        void remove(const void* p);
        void print();
};

extern CObjectTracer*   g_objectTracer;

extern void initObjectTracer();
extern void exitObjectTracer();
extern void insertObjectTracer(const char* name, const void* p);
extern void removeObjectTracer(const void* p);
extern void printObjectTracer();

#else /* DEBUG */

static inline void initObjectTracer() {}
static inline void exitObjectTracer() {}
static inline void insertObjectTracer(const char* name, const void* p) {}
static inline void removeObjectTracer(const void* p) {}
static inline void printObjectTracer() {}

#endif /* DEBUG */

#endif /* __CARBON_OBJECT_TRACER_H_INCLUDED__ */
