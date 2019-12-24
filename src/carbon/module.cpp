/*
 *  Carbon framework
 *  Global Module class
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 12.01.2017 11:55:20
 *      Initial revision.
 */

#include "shell/shell.h"

#include "carbon/module.h"

/*******************************************************************************
 * CModule class
 */

CModule::CModule() : CObject("no-name")
{
}

CModule::CModule(const char* strName) : CObject(strName)
{
}

CModule::~CModule()
{
}

/*******************************************************************************/

/*
 * Initialise array of modules (in order of the array)
 *
 * Return: ESUCCESS, ...
 */
result_t appInitModules(CModule* arModule[], size_t count)
{
	ssize_t     i, j;
	result_t    nresult = ESUCCESS;

	for(i=0; (size_t)i<count; i++)  {
		shell_assert(arModule[i]);

		if ( arModule[i] )  {
			nresult = arModule[i]->init();
			if ( nresult != ESUCCESS )  {
				/* UNDO */
				for(j=i-1; j>=0 ; j--)  {
					if ( arModule[j] ) {
						arModule[j]->terminate();
					}
				}
				break;
			}
		}
	}

	return nresult;
}

/*
 * Termionate module array (in reverce order)
 */
void appTerminateModules(CModule* arModule[], size_t count)
{
	ssize_t		i;

	for(i=count-1; i>=0; i--)  {
		if ( arModule[i] )  {
			arModule[i]->terminate();
		}
	}
}
