/*
 *	Carbon Framework Examples
 *	Linux driver communication
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 27.01.2017 17:49:04
 *	    Initial revision.
 */

#ifndef __EXAMPLE_DRIVER_APP_H_INCLUDED__
#define __EXAMPLE_DRIVER_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"

#define EV_DRIVER_MESSAGE				(EV_USER+0)

class CDriverModule;

class CDriverApp : public CApplication
{
    protected:
        CDriverModule*    m_pDriver;		/* Driver interface module */

    public:
		CDriverApp(int argc, char* argv[]);
        virtual ~CDriverApp();

	public:
        virtual result_t init();
        virtual void terminate();

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);
};


#endif /* __EXAMPLE_DRIVER_APP_H_INCLUDED__ */
