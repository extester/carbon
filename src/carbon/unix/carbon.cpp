/*
 *  Carbon framework
 *  Library definitions, types and includes (UNIX)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 15.02.2017 17:19:00
 *      Initial revision.
 */

#include "carbon/memory.h"
#include "carbon/event/remote_event_service.h"
#include "carbon/carbon.h"

static CMemoryManager* g_pMemoryManager = 0;

CMemoryManager* appMemoryManager()
{
    if ( !g_pMemoryManager )  {
        log_error(L_GEN, "[carbon] memory manager is not initialised!\n");
        return 0;
    }

    return g_pMemoryManager;
}


/*
 * Carbon library bootstrap initialisation
 */
void carbon_init()
{
    result_t    nresult;

    /* Initialise memory manager */
    shell_assert(!g_pMemoryManager);
    g_pMemoryManager = new CMemoryManager();
    nresult = g_pMemoryManager->init();
    shell_assert(nresult == ESUCCESS); UNUSED(nresult);

    /* Initialise logger including STDOUT appender, all levels/channels are enabled */
    logger_init(TRUE);
    logger_enable(LT_ALL|L_ALL);
}

/*
 * Carbon library termination
 */
void carbon_terminate()
{
    if ( g_pMemoryManager )  {
        //g_pMemoryManager->dump();
        g_pMemoryManager->terminate();
    }
    SAFE_DELETE(g_pMemoryManager);

    logger_terminate();
}

result_t appSendRemoteEvent(CRemoteEvent* pEvent, const char* ridDest,
                            CEventReceiver* pReceiver, seqnum_t nSessId)
{
    result_t    nresult = EFAULT;

    shell_assert_ex(g_pRemoteEventService != 0, "Remote Event Service is not initialised");
    if ( g_pRemoteEventService )  {
        nresult = g_pRemoteEventService->sendEvent(pEvent, ridDest, pReceiver, nSessId);
    }
    else {
        pEvent->release();
    }

    return nresult;
}
