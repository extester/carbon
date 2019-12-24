/*
 *	Carbon Framework Examples
 *	Execute external program
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 05.08.2016 17:16:46
 *	    Initial revision.
 */

#ifndef __EXAMPLE_SHELL_EXECUTE_APP_H_INCLUDED__
#define __EXAMPLE_SHELL_EXECUTE_APP_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"

class CShellExecuteApp : public CApplication
{
	private:
		seqnum_t		m_sessId;

    public:
		CShellExecuteApp(int argc, char* argv[]);
        virtual ~CShellExecuteApp();

	public:
        virtual result_t init();
        virtual void terminate();

    protected:
        virtual boolean_t processEvent(CEvent* pEvent);

	private:
		void dumpEvent(CRemoteEvent* pEvent) const;
};


#endif /* __EXAMPLE_SHELL_EXECUTE_APP_H_INCLUDED__ */
