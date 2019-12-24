/*
 *  Carbon framework
 *  Library common definitions, types and includes
 *
 *  Copyright (c) 2015-2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 29.05.2015 18:12:51
 *      Initial revision
 *
 *  Revision 2.0, 15.02.2017 17:03:14
 *  	Separated to common and machine-dependent parts
 */

#ifndef __CARBON_H_INCLUDED__
#define __CARBON_H_INCLUDED__

#include "shell/config.h"
#include "shell/shell.h"
#include "shell/object.h"
#include "shell/dec_ptr.h"

#include "carbon/logger.h"

#ifndef CARBON_PATH_MAX
#define CARBON_PATH_MAX					256
#endif /* CARBON_PATH_MAX */

/*
 * Predefined service RIDs
 */

#define CARBON_SHELL_EXECUTE_RID		"carbon.shell_execute"

/*
 * Session IDs
 */
typedef uint32_t		seqnum_t;
#define NO_SEQNUM		((seqnum_t)0)

extern seqnum_t getUniqueId();

class CEvent;
class CEventLoop;
class CRemoteEvent;
class CMemoryManager;
class CEventReceiver;

extern void carbon_init();						/* machine dependent */
extern void carbon_terminate();					/* machine dependent */

extern version_t carbon_get_version();

extern void appSendEvent(CEvent* pEvent);
extern void appSendMulticastEvent(CEvent* pEvent, CEventLoop* pLoop);
extern result_t appSendRemoteEvent(CRemoteEvent* pEvent, const char* ridDest,
					CEventReceiver* pReceiver = 0, seqnum_t nSessId = NO_SEQNUM);

extern CEventLoop* appMainLoop();
extern CMemoryManager* appMemoryManager();

#endif /* __CARBON_H_INCLUDED__ */
