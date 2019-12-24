/*
 *  Shell library
 *  Object counters support
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 14.06.2015 19:45:56
 *      Initial revision.
 *
 */

#ifndef __SHELL_TRACK_OBJECT_H_INCLUDED__
#define __SHELL_TRACK_OBJECT_H_INCLUDED__

#include <algorithm>

#include "shell/types.h"
#include "shell/logger.h"
#include "shell/lockedlist.h"

template<typename T>
class CTrackObject
{
    private:
        static RLockedList<T*>   m_arObject;

    public:
        CTrackObject() { insert((T*)this); }
        CTrackObject(const CTrackObject&) { insert((T*)this); }
        virtual ~CTrackObject() { remove((T*)this); }

    private:
        void insert(T* object) {
            CAutoLock   locker(m_arObject);
            m_arObject.insert_last(object);
        }

        static void remove(T* object) {
            CAutoLock   locker(m_arObject);
            typename RLockedList<T*>::iterator it;

            it = std::find(m_arObject.begin(), m_arObject.end(), object);
            if ( it != m_arObject.end() ) {
                m_arObject.erase(it);
            }
        }

    public:
        static size_t getTrackCount() {
            CAutoLock   locker(m_arObject);
            return m_arObject.size();
        }

        void dumpTrackObjects() {
            CAutoLock   locker(m_arObject);
            typename RLockedList<T*>::iterator it;

            for(it = m_arObject.begin(); it != m_arObject.end(); it++) {
                (*it)->dump();
            }
        }

        static void check(const char* strObjects, boolean_t bAssert = FALSE);
};

template<typename T>
RLockedList<T*> CTrackObject<T>::m_arObject;

template<typename T>
void CTrackObject<T>::check(const char* strObjects, boolean_t bAssert) {
    CAutoLock   locker(m_arObject);
    typename RLockedList<T*>::iterator it;

    if ( m_arObject.size() != 0 ) {
        log_dump("--- WARNING: There are %u existing %s objects! ---\n",
                 m_arObject.size(), strObjects);

        for(it = m_arObject.begin(); it != m_arObject.end(); it++) {
            (*it)->dump();
        }

        if ( bAssert )  {
            shell_assert_ex(m_arObject.size() != 0, "existing objects %s", strObjects);
        }
    }
    /*else {
        log_dump("--- Track Objects '%s': success ---\n", strObjects);
    }*/
}

/*******************************************************************************/

/*
template<typename T>
class CObjectCounter
{
    private:
        static atomic_t     m_nCount;

    public:
        CObjectCounter() { atomic_inc(&m_nCount); }
        CObjectCounter(const CObjectCounter&) { atomic_inc(&m_nCount); }
        virtual ~CObjectCounter() { atomic_dec(&m_nCount); }

    public:
        static size_t getCount() { return (size_t)atomic_get(&m_nCount); }
};

template<typename T>
atomic_t CObjectCounter<T>::m_nCount = ZERO_ATOMIC;
*/

#endif /* __SHELL_TRACK_OBJECT_H_INCLUDED__ */
