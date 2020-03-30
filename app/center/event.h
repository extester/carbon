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
 *  Revision 1.0, 02.06.2015 14:34:47
 *      Initial revision.
 */

#ifndef __CENTER_EVENT_H_INCLUDED__
#define __CENTER_EVENT_H_INCLUDED__

#include "carbon/event.h"

#define EV_HEARTBEAT_REQUEST			(EV_USER+0)
#define EV_HEARTBEAT_REPLY				(EV_USER+1)


extern void initCenterEvent();

#endif /* __CENTER_EVENT_H_INCLUDED__ */
