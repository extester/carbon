/*
 *	Carbon Framework Examples
 *	Minimal program
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 01.06.2016 10:51:22
 *	    Initial revision.
 */

#ifndef ___EXAMPLE_MINIMAL_H_INCLUDED__
#define ___EXAMPLE_MINIMAL_H_INCLUDED__

#include "carbon/carbon.h"
#include "carbon/application.h"


class CMinimalApp : public CApplication
{
    public:
        CMinimalApp(int argc, char* argv[]);
        virtual ~CMinimalApp();

	public:
        virtual result_t init();
        virtual void terminate();
};


#endif /* ___EXAMPLE_MINIMAL_H_INCLUDED__ */
