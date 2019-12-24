/*
 *	Carbon Framework Examples
 *	Keyboard module
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 26.01.2017 12:22:57
 *	    Initial revision.
 */

#ifndef __EXAMPLE_KEYBOARD_H_INCLUDED__
#define __EXAMPLE_KEYBOARD_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/module.h"
#include "carbon/event.h"
#include "carbon/thread.h"

#include "keyboard_driver.h"

class CModuleApp;
class CReceiverModule;

class CKeyboardModule : public CModule
{
    protected:
        CModuleApp*	const		m_pParent;			/* Parent application */
		CReceiverModule* const	m_pReceiver;		/* Key processor */
		CThread					m_worker;			/* Worker thread */
		CKeyboardDriver*		m_pHwKeyboard;		/* Low-level keyboard driver */

    public:
        CKeyboardModule(CModuleApp* pParent, CReceiverModule* pReceiver);
        virtual ~CKeyboardModule();

	public:
        virtual result_t init();
        virtual void terminate();

		virtual void dump(const char* strPref = "") const;

    private:
		void* workerThread(CThread* pThread, void* pData);
};


#endif /* __EXAMPLE_KEYBOARD_H_INCLUDED__ */
