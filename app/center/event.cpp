/*
 *  Carbon Framework Network Center
 *  Custom events
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 02.06.2015 14:37:39
 *      Initial revision.
 */

#include "shell/shell.h"

#include "event.h"


static const char* stable[] = {
	"EV_HEARTBEAT_REQUEST",
	"EV_HEARTBEAT_REPLY"
};

void initCenterEvent()
{
	result_t	nresult;

	nresult = CEventStringTable::registerStringTable(EV_HEARTBEAT_REQUEST, EV_HEARTBEAT_REPLY, stable);
	shell_assert(nresult == ESUCCESS); shell_unused(nresult);
}
