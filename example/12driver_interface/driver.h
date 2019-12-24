/*
 *	Carbon Framework Examples
 *	Linux driver communication module
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 27.01.2017 18:19:33
 *	    Initial revision.
 */

#ifndef __EXAMPLE_DRIVER_H_INCLUDED__
#define __EXAMPLE_DRIVER_H_INCLUDED__

#include "shell/file.h"

#include "carbon/carbon.h"
#include "carbon/module.h"
#include "carbon/event.h"
#include "carbon/thread.h"

class CDriverApp;

class CDriverModule : public CModule
{
    protected:
        CDriverApp*	const		m_pParent;			/* Parent application */
		CThread					m_worker;			/* Worker thread */
		CFile					m_driver;

    public:
		CDriverModule(CDriverApp* pParent);
        virtual ~CDriverModule();

	public:
        virtual result_t init();
        virtual void terminate();

		virtual void dump(const char* strPref = "") const;

    private:
		void* workerThread(CThread* pThread, void* pData);
};


#endif /* __EXAMPLE_DRIVER_H_INCLUDED__ */
