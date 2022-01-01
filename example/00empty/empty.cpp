/*
 *  Carbon Framework
 *  Empty application for the education purpose
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 24.01.2017 11:19:47
 *		Initial revision.
 */

#include "carbon/application.h"

int main(int argc, char* argv[])
{
    CApplication*   pApp;
    int	            nOsExitCode;

    pApp = new CApplication("Empty App", MAKE_VERSION(0,0,1), 1, argc, argv);
	nOsExitCode = pApp->run();
    delete pApp;

    return nOsExitCode;
}
