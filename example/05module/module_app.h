/*
 *	Carbon Framework Examples
 *	Main application
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 26.01.2017 11:48:17
 *	    Initial revision.
 */

#ifndef __EXAMPLE_MODULE_APP_H_INCLUDED__
#define __EXAMPLE_MODULE_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"

class CReceiverModule;
class CKeyboardModule;

class CModuleApp : public CApplication
{
    protected:
        CReceiverModule*    m_pReceiver;		/* Key press process module */
		CKeyboardModule*	m_pKeyboard;		/* Key press generate module */

    public:
        CModuleApp(int argc, char* argv[]);
        virtual ~CModuleApp();

	public:
        virtual result_t init();
        virtual void terminate();
};


#endif /* __EXAMPLE_MODULE_APP_H_INCLUDED__ */
