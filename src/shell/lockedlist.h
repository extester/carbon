/*
 *  Shell library
 *  Locked list
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 27.03.2013 12:31:08
 *      Initial revision.
 */

#ifndef __SHELL_LOCKEDLIST_H_INCLUDED__
#define __SHELL_LOCKEDLIST_H_INCLUDED__

#include <list>

#include "shell/lock.h"

template <class Type>
class RLockedList : public std::list<Type>
{
    private:
        CMutex m_lock;

    public:
        RLockedList() : m_lock(CMutex::mutexRecursive) {}
        virtual ~RLockedList() {}

        typedef typename std::list<Type>::iterator iterator;
        iterator remove(iterator iter) { return this->erase(iter); }

        operator CLock&() { return m_lock; }

        void lock() { m_lock.lock(); }
        void unlock() { m_lock.unlock(); }

        void insert_last(Type item)
        {
            this->push_back(item);
        }

        Type remove_first()
        {
            Type item = this->front();
            this->pop_front();
            return item;
        }

        size_t count() const
        {
            return this->size();
        }

        boolean_t is_empty() const
        {
            return this->empty();
        }
};

class CListItem
{
    protected:
        CListItem       *m_pNext, *m_pPrev;

    public:
        CListItem() : m_pNext(0), m_pPrev(0) {}
        virtual ~CListItem() {}

    public:
        CListItem* next() { return m_pNext; }
        CListItem* prev() { return m_pPrev; }
        boolean_t isFirst() const { return m_pNext == 0; }
        boolean_t isLast() const { return m_pPrev == 0; }

        void setNext(CListItem* pItem) { m_pNext = pItem; }
        void setPrev(CListItem* pItem) { m_pPrev = pItem; }
};

template <class T>
class CLockedList : public CMutex
{
    private:
        CListItem       *m_pHead, *m_pTail;
        size_t          m_count;

    public:
        CLockedList() : CMutex(), m_pHead(0), m_pTail(0), m_count(0) {
		}
        virtual ~CLockedList() {}

    public:
        T* getFirst() const {
            return (T*)(m_pHead ? m_pHead : 0);
        }

        T* getNext(T* pItem) const {
            return (T*)(pItem->isLast() ? 0 : pItem->prev());
        }

        size_t getSize() const { return m_count; }
		boolean_t isEmpty() const { return getSize() == 0; }
        T* getHead() const { return (T*)m_pHead; }
        T* getTail() const { return (T*)m_pTail; }

        void insert(T* pItem)  {
            if ( m_pTail )  {
                shell_assert(m_pTail->prev() == 0);

                m_pTail->setPrev(pItem);
                pItem->setPrev(0);
                pItem->setNext(m_pTail);

                m_pTail = pItem;
            }
            else {
                m_pHead = m_pTail = pItem;
                pItem->setNext(0);
                pItem->setPrev(0);
            }
            m_count++;
        }

		void insert_before(T* pItemCur, T* pItem) {
			if ( pItemCur->next() != 0 )  {
				pItem->setNext(pItemCur->next());
				pItemCur->setNext(pItem);

				pItem->setPrev(pItem->next()->prev());
				pItem->next()->setPrev(pItem);
			}
			else {
				pItemCur->setNext(pItem);
				pItem->setNext(0);

				pItem->setPrev(pItemCur);
				m_pHead = pItem;
			}
			m_count++;
		}

        void remove(T* pItem) {
            if ( pItem == m_pHead )  {
                if ( m_pHead->isLast() ) {
                    m_pHead = m_pTail = 0;
                }
                else {
                    m_pHead = m_pHead->prev();
                    m_pHead->setNext(0);
                }
            }
            else {
                if ( pItem == m_pTail )  {
                    if ( m_pTail->isFirst() )  {
                        m_pHead = m_pTail = 0;
                    }
                    else {
                        m_pTail = m_pTail->next();
                        m_pTail->setPrev(0);
                    }
                }
                else {
                    pItem->prev()->setNext(pItem->next());
                    pItem->next()->setPrev(pItem->prev());
                }
            }
            m_count--;
        }

		boolean_t find(const T* pItem) const {
			boolean_t	bFound = FALSE;
			T*			pCurItem = getFirst();

			while ( pItem != 0 )  {
				if ( pCurItem == pItem )  {
					bFound = TRUE;
					break;
				}

				pCurItem = getNext(pCurItem);
			}

			return bFound;
		}
};

#endif /* __SHELL_LOCKEDLIST_H_INCLUDED__ */
