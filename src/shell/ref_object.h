/*
 *  Shell library
 *  Reference counter for objects support
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.06.2015 11:15:00
 *      Initial revision.
 *
 */

#ifndef __SHELL_REF_OBJECT_H_INCLUDED__
#define __SHELL_REF_OBJECT_H_INCLUDED__

#include "shell/types.h"
#include "shell/assert.h"
#include "shell/atomic.h"

class CRefObject
{
    protected:
        mutable atomic_t    m_nRefCount;
        boolean_t   		m_bDeleted;

    protected:
        CRefObject() {
            sh_atomic_set(&m_nRefCount, 1);
            m_bDeleted = FALSE;
        }

        virtual ~CRefObject() {}

    public:
        virtual int getRefCount() const { return sh_atomic_get(&m_nRefCount); }
        virtual int reference() { return sh_atomic_inc(&m_nRefCount); }
		virtual int release() {
			int nRefCount = getRefCount();

			shell_assert(nRefCount > 0);
			if ( (nRefCount = sh_atomic_dec(&m_nRefCount)) <= 0 ) {
				delete this;
			}

			return nRefCount;
		}

        void markDeleted() { m_bDeleted = TRUE; }
        boolean_t isDeleted() const { return m_bDeleted; }
};

#endif /* __SHELL_REF_OBJECT_H_INCLUDED__ */
