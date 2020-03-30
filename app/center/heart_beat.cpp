/*
 *  Carbon Framework Network Center
 *  Hosts checker
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 02.06.2015 13:07:48
 *      Initial revision.
 */

#include <errno.h>

#include "contact/icmp.h"

#include "center.h"
#include "event.h"
#include "heart_beat.h"

#define HEARTBEAT_ITEM_MAX              100
#define HEARTBEAT_PING_ITERATIONS       3
#define HEARTBEAT_PING_TIMEOUT          HR_3SEC


/*******************************************************************************
 * CHeartBeat class
 */

CHeartBeat::CHeartBeat(CCenterApp* pParent, const CNetAddr& srcAddr) :
    CModule("HeartBeat"),
    CEventLoopThread("HeartBeat"),
    CEventReceiver(this, "HeartBeat"),
    m_pParent(pParent),
    m_srcAddr(srcAddr)
{
}

CHeartBeat::~CHeartBeat()
{
}

result_t CHeartBeat::runPing(heart_beat_request_t* pTable, size_t count)
{
    CICmp               pinger("HeartBeat");
    ping_request_t      *arPing, *pPing;
    size_t              i, j;
    result_t            nresult;

    nresult = ESUCCESS;
    try {
		arPing = new ping_request_t[count];
	}
	catch(const std::bad_alloc& exc)  {
		log_error(L_NETCONN, "[heartbeat] out of memory\n");
		nresult = ENOMEM;
	}

	if ( nresult != ESUCCESS )  {
		return nresult;
	}

    for (i=0; i<count; i++) {
        pPing = &arPing[i];
        pPing->dstHost = CNetHost(pTable[i].dstAddr);
        pPing->arTimes = (hr_time_t*)memAlloca(HEARTBEAT_PING_ITERATIONS * sizeof(hr_time_t));
    }

    nresult = pinger.multiPing(arPing, (int)count, HEARTBEAT_PING_ITERATIONS,
                               HEARTBEAT_PING_TIMEOUT, m_srcAddr);
    if ( nresult == ESUCCESS ) {
        /*
         * Calculate average times
         */
        for (i=0; i<count; i++) {
            hr_time_t hrAccum = ICMP_ITER_FAILED, hr;
            int num = 0;

            pPing = &arPing[i];
            for (j=0; j<HEARTBEAT_PING_ITERATIONS; j++) {
				hr = pPing->arTimes[j];
                if ( hr != ICMP_ITER_NOTSET && hr != ICMP_ITER_FAILED) {
                    hrAccum = hrAccum != ICMP_ITER_FAILED ?
                              (hrAccum + hr) : hr;
                    num++;
                }
            }

            pTable[i].hrAverage = num > 0 ? (hrAccum/num) : ICMP_ITER_FAILED;
        }
    }
    else {
        if ( nresult != EINTR ) {
            log_debug(L_GEN, "[heartbeat] multiping failed, result: %d\n", nresult);
        }
    }

    delete [] arPing;

    return nresult;
}

void CHeartBeat::processRequest(heart_beat_request_t* pTable, size_t count)
{
    CEvent*     pEvent;
    result_t    nresult;

    if ( count < HEARTBEAT_ITEM_MAX )  {
        //log_debug(L_GEN, "[hb] running ping, count: %d\n", count);
        nresult = runPing(pTable, count);
    }
    else {
        log_debug(L_GEN, "[hb] too many request items %u, limit %d\n",
                    count, HEARTBEAT_ITEM_MAX);
        nresult = E2BIG;
    }

    /* Send reply */
    pEvent = new CEvent(EV_HEARTBEAT_REPLY, m_pParent->getHostList(),
                        (PPARAM)0, (NPARAM)nresult, "");
    appSendEvent(pEvent);
    (void)pEvent;
}

/*
 * Event processor
 *
 *      type        event type, EV_xxx
 */
boolean_t CHeartBeat::processEvent(CEvent* pEvent)
{
    boolean_t   bProcessed = FALSE;

    switch ( pEvent->getType() )  {
        case EV_HEARTBEAT_REQUEST:
            processRequest((heart_beat_request_t*)pEvent->getpParam(), (size_t)pEvent->getnParam());
            bProcessed = TRUE;
            break;
    }

    return bProcessed;
}

/*
 * Start module
 *
 * Return: ESUCCESS, ...
 */
result_t CHeartBeat::init()
{
    result_t    nresult;

    nresult = CEventLoopThread::start();
    return nresult;
}

/*
 * Stop module
 */
void CHeartBeat::terminate()
{
    CEventLoopThread::stop();
}
