/*
 *	Carbon Framework Examples
 *	Remote Event shared definitions
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 03.08.2016 17:29:33
 *	    Initial revision.
 */

#ifndef __SHARED_H_INCLUDED__
#define __SHARED_H_INCLUDED__

#include "carbon/event.h"

#define CARBON_REMOTE_EVENT_RECV_RID	"carbon.example.remote_event.recv"
#define CARBON_REMOTE_EVENT_SEND_RID    "carbon.example.remote_event.send"

#define EV_R_TEST						(EV_USER+0)
#define EV_R_TEST_REPLY					(EV_USER+1)

#endif /* __SHARED_H_INCLUDED__ */
