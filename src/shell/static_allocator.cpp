/*
 *  Shell library
 *  Static buffers allocator
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 * Note: Static memory allocations
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 04.04.2017 15:08:28
 *      Initial revision.
 */

#include "shell/static_allocator.h"

void no_static_memory()
{
	log_error(L_ALWAYS_ENABLED, " === MEMORY ALLOCATION FAILED === \n");
	shell_assert(FALSE);
	while ( TRUE )  {}
}

