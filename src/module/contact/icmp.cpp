/*
 *  Carbon/Contact module
 *  ICMP protocol related classes and functions
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 24.05.2015 20:35:38
 *      Initial revision.
 */
/*
 * 	struct iphdr {
 *		unsigned int 	ihl:4;
 *		unsigned int 	version:4;
 *		u_int8_t 		tos;
 *		u_int16_t 		tot_len;
 *		u_int16_t 		id;
 *		u_int16_t 		frag_off;
 *		u_int8_t 		ttl;
 *		u_int8_t 		protocol;
 *		u_int16_t 		check;
 *		u_int32_t 		saddr;
 *		u_int32_t 		daddr;
 *	};
 *
 *  struct icmphdr {
 *		u_int8_t 		type;			message type
 *		u_int8_t		code;			type sub-code
 *		u_int16_t		checksum;
 *		union
 *		{
 *			struct
 *			{
 *				u_int16_t	id;
 *				u_int16_t	sequence;
 *			} echo;						echo datagram
 *			u_int32_t	gateway;		gateway address
 *			struct
 *			{
 *				u_int16_t	__unused;
 *				u_int16_t	mtu;
 *			} frag;						path mtu discovery
 *		} un;
 *	};
 */

#include <sys/socket.h>
#include <poll.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/memory.h"
#include "shell/hr_time.h"
#include "shell/net/netutils.h"
#include "shell/utils.h"

#include "contact/icmp.h"

#define ICMP_MULTI_MAX                  (16*1024)


/*******************************************************************************
 * ICMP protocol class
 */

CICmp::CICmp(const char* strTitle) :
    m_ipIdent(1),
    m_icmpSeq(1)
{
    struct ip*      pIp;
    struct icmp*	pIcmp;

    copyString(m_strTitle, strTitle, sizeof(m_strTitle));
    m_icmpIdent = (uint16_t)random();

    shell_assert(sizeof(m_templ) >= ICMP_PACKET_TOTAL_LENGTH);

    _tbzero(&m_templ, ICMP_PACKET_TOTAL_LENGTH);

    /* Fill IP header template */
    pIp = (struct ip*)m_templ;

    pIp->ip_hl = sizeof(struct ip)>>2;		    /* IP header length in 32-bit dwords */
    pIp->ip_v = 4;							    /* IP protocol version */
    /*pIp->ip_tos = 0;*/						/* Type of Service */
    pIp->ip_len = htons(ICMP_PACKET_TOTAL_LENGTH);
    //pIp->ip_id = 0;
    /*pIp->ip_off = 0;*/
    pIp->ip_ttl = 255;
    pIp->ip_p = IPPROTO_ICMP;
    /*pIp->ip_sum = 0;*/    /* Filled by kernel */
    //pIp->ip_src = ;
    //pIp->ip_dst = ;

    /* Fill ICMP header template */
    pIcmp = (struct icmp*)(m_templ + sizeof(struct ip));

    pIcmp->icmp_type = ICMP_ECHO;
    /*pIcmp->icmp_code = 0;*/
    /*pIcmp->icmp_cksum = 0;*/
    //pIcmp->icmp_id = htons(icmp_ident);
    //pIcmp->icmp_seq = htons(icmp_sequence);
    //icmp->cksum = in_cksum((uint16*)icmp, sizeof(struct icmp));
}

CICmp::~CICmp()
{
}

/*
 * Process echo reply
 *
 *      pPing           ping data pointer
 *      index           ping iteration index
 *
 * Return:
 *      TRUE            reply processed successful
 *      FALSE           unknown reply, continue receiving
 */
boolean_t CICmp::checkReply(ping_request_t* pPing, int index) const
{
    struct icmp*    pIcmp;

    shell_assert(pPing->ioLen == ICMP_PACKET_TOTAL_LENGTH);

    pIcmp = (struct icmp*)(pPing->ioBuffer + sizeof(struct ip));
    if ( ntohs(pIcmp->icmp_id) != pPing->icmpIdent ||
            ntohs(pIcmp->icmp_seq) != pPing->icmpSeq )
    {
        /*
         * Wrong/unknown ICMP packet received
         */
		log_trace(L_ICMP, "[icmp_io(%d)] %s: ignored unknown packet: id %u (expected %u), "
                        "seq %u (expected %u) from %s\n",
                        index, m_strTitle, ntohs(pIcmp->icmp_id), pPing->icmpIdent,
                        ntohs(pIcmp->icmp_seq), pPing->icmpSeq,
                        (const char*)pPing->dstHost);
        return FALSE;
    }

    if ( pIcmp->icmp_type != ICMP_ECHOREPLY ) {
		log_trace(L_ICMP, "[icmp(%d)] %s: wrong ICMP packet type %u received (expected %u) "
                        "from %s, ignored\n", index, m_strTitle,
                        pIcmp->icmp_type, ICMP_ECHOREPLY,
                        (const char*)pPing->dstHost);
        return FALSE;
    }

    /*
     * Ping succeeded
     */
    return TRUE;
}

/*
 *
 *
 *
 * Return:
 *      ESUCCESS        succeeded and completed, no pending
 *      EBUSY           succeeded and incompleted, pending
 *      EINTR           interrupted by signal, no pending
 *      other           failed and completed, no pending
 */
result_t CICmp::processActivity(ping_request_t* pPing, int index)
{
    struct ip*      pIp;
    struct icmp*    pIcmp;
    size_t          size;
    result_t        nresult, nr;
    boolean_t       bResult;

    switch ( pPing->stage ) {
        case CICmp::echoIdle:   /* Start sending a new icmp ECHO packet */
            //log_debug(L_ICMP_FL, "[cmp_io] start sending, id: %u, seq: %u\n", m_icmpIdent, m_icmpSeq);
            UNALIGNED_MEMCPY(pPing->ioBuffer, &m_templ, ICMP_PACKET_TOTAL_LENGTH);
            pIp = (struct ip*)pPing->ioBuffer;
            pIp->ip_id = htons(m_ipIdent);
            pIp->ip_dst = pPing->dstHost;

            pIcmp = (struct icmp*)(pPing->ioBuffer + sizeof(struct ip));
            pIcmp->icmp_id = htons(m_icmpIdent);
            pIcmp->icmp_seq = htons(m_icmpSeq);
            pIcmp->icmp_cksum = in_cksum((uint16_t*)pIcmp, sizeof(struct icmp));

            pPing->ioLen = 0;
            pPing->stage = CICmp::echoSending;
            m_ipIdent++;
            pPing->icmpIdent = m_icmpIdent++;
            pPing->icmpSeq = m_icmpSeq++;
            /* fall down */

        case CICmp::echoSending:
            //log_debug(L_ICMP_FL, "[cmp_io] continue sending, offset: %u\n", pPing->ioLen);
            if ( pPing->ioLen < ICMP_PACKET_TOTAL_LENGTH) {
                size = ICMP_PACKET_TOTAL_LENGTH - pPing->ioLen;

                nr = pPing->socket.sendAsync(&pPing->ioBuffer[pPing->ioLen],
                                          &size, pPing->dstHost);
                /*log_debug(L_ICMP_FL, "[cmp_io] continue sending, sent size: %d result: %d (%s)\n",
                          size, nr, strerror(nr));*/
                pPing->ioLen += size;

                if ( nr != ESUCCESS && nr != EAGAIN && nr != EWOULDBLOCK ) {
                    /* Hard error (or EINTR) on sending */
                    nresult = nr;
                    if ( nr != EINTR ) {
                        log_debug(L_ICMP, "[icmp_io(%d)] %s: error on sending ECHO request "
                                "to %s, result: %d\n", index, m_strTitle,
                                (const char*)pPing->dstHost, nr);
                    }

                    pPing->stage = CICmp::echoDone;
                    break;
                }
            }

            if ( pPing->ioLen < ICMP_PACKET_TOTAL_LENGTH ) {
                /* Continue sending */
                nresult = EBUSY;
                break;
            }

            pPing->stage = CICmp::echoReceiving;
            pPing->ioLen = 0;
            /* fall down */

        case CICmp::echoReceiving:
            //log_debug(L_ICMP_FL, "[cmp_io] continue receiving, offset: %u\n", pPing->ioLen);
            if ( pPing->ioLen < ICMP_PACKET_TOTAL_LENGTH) {
                size = ICMP_PACKET_TOTAL_LENGTH - pPing->ioLen;

                nr = pPing->socket.receiveAsync(&pPing->ioBuffer[pPing->ioLen], &size);
				/*log_debug(L_ICMP, "[cmp_io] continue receiving, recv size: %d result: %d (%s)\n",
                          size, nr, strerror(nr));*/
                pPing->ioLen += size;

                if ( nr != ESUCCESS && nr != EAGAIN && nr != EWOULDBLOCK ) {
                    nresult = nr;
                    if ( nr != EINTR ) {
                        log_debug(L_ICMP, "[icmp_io(%d)] %s: error on receiving ECHO reply "
                                "from %s, result: %d\n", index, m_strTitle,
                                (const char*)pPing->dstHost, nr);
                    }

                    pPing->stage = CICmp::echoDone;
                    break;
                }
            }

            if ( pPing->ioLen < ICMP_PACKET_TOTAL_LENGTH ) {
                /* Continue receiving */
                nresult = EBUSY;
                break;
            }

            /*
             * Received a reply
             */
            /*{
                pIp = (struct ip *) pPing->ioBuffer;
                CNetHost src(pIp->ip_src), dst(pIp->ip_dst);

                log_debug(L_ICMP, "[cmp_io] received REPLY, %s => %s\n",
                          (const char *) src, (const char *) dst);
            }*/

            bResult = checkReply(pPing, index);
            if ( bResult ) {
                /* Ping completed */
                pPing->stage = CICmp::echoDone;
                nresult = ESUCCESS;
            }
            else {
                /* Continue receiving */
                pPing->ioLen = 0;
                nresult = EBUSY;
            }
            break;

        case CICmp::echoDone:
        default:
            shell_assert_ex(FALSE, "stage=%d\n", pPing->stage);
            nresult = EINVAL;
            break;
    }

    return nresult;
}

/*
 * Make a single send/receive iteration
 *
 *      index           iteration index
 *      hrTimeout       maximum iteration time
 *      arPing          ping array
 *      count           ping array items
 *
 * Return:
 *      ESUCCESS        success
 *      EINTR           interrupted by signal
 *
 */
result_t CICmp::doSinglePing(int index, hr_time_t hrTimeout, ping_request_t* arPing, int count)
{
    struct pollfd	    *arPoll, *pPoll;
    ping_request_t*     pPing;
    hr_time_t           hrElapsed, hrStart;
    int                 msTimeout, i, n, nPending;
    result_t            nresult, nr;

    shell_assert(index >= 0);
    shell_assert(hrTimeout);
    shell_assert(count > 0);

    arPoll = (struct pollfd*)memAlloca(count*sizeof(struct pollfd));

    nPending = 0;
    for(i=0; i<count; i++) {
        pPing = &arPing[i];

        if ( pPing->socket.isOpen() ) {
            pPing->stage = CICmp::echoIdle;
            nPending++;
        }
        else {
            pPing->stage = CICmp::echoDone;
        }
    }

    nresult = ESUCCESS;
    hrStart = hr_time_now();

    while ( (hrElapsed=hr_time_get_elapsed(hrStart)) < hrTimeout &&
            nPending > 0 && nresult == ESUCCESS )
    {
        msTimeout = (int)HR_TIME_TO_MILLISECONDS(hrTimeout-hrElapsed);
        if ( msTimeout < 1 )  {
            break;      /* Timeout has been expired */
        }

        for(i=0; i<count; i++)  {
            pPing = &arPing[i];
            pPoll = &arPoll[i];

            switch (pPing->stage) {
                case CICmp::echoIdle:
                case CICmp::echoSending:
                    pPoll->events = POLLOUT;
                    pPoll->revents = 0;
                    pPoll->fd = pPing->socket.getHandle();
                    break;

                case CICmp::echoReceiving:
                    pPoll->events = POLLIN|POLLPRI;
                    pPoll->revents = 0;
                    pPoll->fd = pPing->socket.getHandle();
                    break;

                default:
                    shell_assert(pPing->stage == CICmp::echoDone);
                    pPoll->fd = -1;
                    break;
            }
        }

        n = poll(arPoll, (nfds_t)count, msTimeout);

        if ( n < 0 ) {
            nresult = errno;
            if ( nresult != EINTR ) {
                log_debug(L_ICMP, "[icmp_io(%d)] %s: poll() failed, result: %d\n",
                          index, m_strTitle, nresult);
            }
            break;
        }

        if ( n == 0 )  {
            break;      /* Timeout has been expired */
        }

        nPending = 0;
        hrElapsed = hr_time_get_elapsed(hrStart);

        for(i=0; i<count; i++)  {

            pPing = &arPing[i];
            pPoll = &arPoll[i];

            if (
                    ((pPing->stage == CICmp::echoIdle || pPing->stage == CICmp::echoSending) &&
                    (pPoll->revents&POLLOUT) != 0)
                    ||
                    (pPing->stage == CICmp::echoReceiving && (pPoll->revents&(POLLIN|POLLPRI)) != 0)
               )
            {
                /*log_debug(L_ICMP_FL, "[icmp_io(%d)] [%d] processActivity: stage: %d\n", index, i, pPing->stage);*/
                nr = processActivity(pPing, index);

                if ( nr == ESUCCESS )  {
                    /* Iteration completed successfully */
                    shell_assert(pPing->stage == CICmp::echoDone);
                    pPing->cntSuccess++;
                    if ( pPing->arTimes )  {
                        pPing->arTimes[index] = hrElapsed;
                    }
                    continue;
                }

                if ( nr == EBUSY ) {
                    /* Iteration is not completed */
                    shell_assert(pPing->stage != CICmp::echoDone);
                    nPending++;

                    /* Check errors */
                    if ( pPoll->revents&(POLLERR|POLLHUP|POLLNVAL) ) {
                        /* Iteration failed */
                        pPing->stage = CICmp::echoDone;
                        pPing->cntFailed++;
                        if ( pPing->arTimes )  {
                            pPing->arTimes[index] = ICMP_ITER_FAILED;
                        }
                        nPending--;

                        log_debug(L_ICMP, "[icmp_io(%d)] %s: poll() return errors:%s%s%s "
                                "from %s, ping iteration cancelled\n",
                                  index, m_strTitle,
                                  (pPoll->revents & POLLERR) ? " POLLERR" : "",
                                  (pPoll->revents & POLLHUP) ? " POLLHUP" : "",
                                  (pPoll->revents & POLLNVAL) ? " POLLNVAL" : "");
                    }
                    continue;
                }

                if ( nr == EINTR )  {
                    /* Interrupted by signal */
                    nresult = nr;
                    break;
                }

                /* Iteration failed */
                shell_assert(pPing->stage == CICmp::echoDone);
                pPing->cntFailed++;
                if ( pPing->arTimes )  {
                    pPing->arTimes[index] = ICMP_ITER_FAILED;
                }
            }
            else {
                nPending += pPing->stage != CICmp::echoDone ? 1 : 0;
            }
        }
    }

    /*
     * Complete iteration
     */
    int nPend = 0;

    for(i=0; i<count; i++)  {
        pPing = &arPing[i];

        if ( pPing->stage != CICmp::echoDone )  {
            pPing->stage = CICmp::echoDone;
            pPing->cntFailed++;
            if ( pPing->arTimes )  {
                pPing->arTimes[index] = ICMP_ITER_FAILED;
            }

            nPend++;
        }
    }

    shell_assert_ex(nPending == nPend, "incorrect pending value: nPending %u, expected %u\n",
                        nPending, nPend);

    return nresult == EINTR ? EINTR : ESUCCESS;
}

/*
 * Multi-Ping
 *
 *      arPing              ping destination array
 *      count               ping array items
 *      iterationCount      ping count
 *      hrTimeout           maximum I/O time per iteration
 *      srcHost             ping source ip address
 *
 * Return:
 * 		ESUCCESS		success (individual results are in the arPing array)
 * 		EINTR			interrupted by signal
 * 		EINVAL          some parameters are invalid
 * 		ETIMEDOUT       iteration time too short
 */
result_t CICmp::doMultiPing(ping_request_t* arPing, int count, int iterationCount,
                            hr_time_t hrTimeout, const CNetHost& srcHost)
{
    ping_request_t* pPing;
    struct ip* 		pip;
    int             i;
    int             nPending;
    result_t        nresult;

    /*
     * Sanity checks
     */
    if ( count < 1 )  {
        return ESUCCESS;
    }

    if ( count > ICMP_MULTI_MAX )  {
        log_error(L_ICMP, "[icmp_io] %s: too many requests (%d) in I/O, limit %d\n",
                  m_strTitle, count, ICMP_MULTI_MAX);
        return EINVAL;
    }

    if ( srcHost == NETHOST_NULL )  {
        log_error(L_ICMP, "[icmp_io] %s: no source address specified\n", m_strTitle);
        return EINVAL;
    }

    if ( hrTimeout == HR_0 )  {
        log_error(L_ICMP, "[icmp_io] %s: zero timeout specified\n", m_strTitle);
        return ETIMEDOUT;
    }

    /*
     * Create communication sockets
     */

    for(i=0; i<count; i++)  {
        arPing[i].cntSuccess = 0;
        arPing[i].cntFailed = 0;
        if ( arPing[i].arTimes )  {
            _tbzero(arPing[i].arTimes, sizeof(arPing[i].arTimes[0])*iterationCount);
        }
    }

    pip = (struct ip*)m_templ;
    pip->ip_src = srcHost;
    nPending = 0;

    for(i=0; i<count; i++)  {
        result_t    nr;

        pPing = &arPing[i];

        nr = pPing->socket.open(NETADDR_NULL, SOCKET_TYPE_RAW);
        if ( nr == ESUCCESS )  {
            nPending++;
        }
        else {
            log_error(L_ICMP, "[icmp(%d)] %s: failed to open raw socket for "
                    "destination %s, result: %d\n", i, m_strTitle,
                    pPing->dstHost.cs(), nr);
        }
    }

    /*
     * Pinging
     */
    nresult = ESUCCESS;
    if ( nPending )  {
        for(i=0; i<iterationCount && nresult==ESUCCESS; i++) {
            nresult = doSinglePing(i, hrTimeout, arPing, count);
        }

        shell_assert(nresult==ESUCCESS || nresult==EINTR);
    }

    for(i=0; i<count; i++)  {
        arPing[i].socket.close();
    }

    return nresult;
}

/*
 * Ping single server
 *
 *      pPing           ping parameters/results
 *      iterations      ping count
 *      hrTimeout       iteration maximum time
 *      srcHost         source address
 *
 * Return:
 *      ESUCCESS        success, check the results
 * 		EINTR			interrupted by signal
 * 		EINVAL          some parameters are invalid
 * 		ETIMEDOUT       iteration time too short
 */
result_t CICmp::ping(ping_request_t* pPing, int iterations, hr_time_t hrTimeout,
                     const CNetHost& srcHost)
{
    return doMultiPing(pPing, 1, iterations, hrTimeout, srcHost);
}

result_t CICmp::multiPing(ping_request_t* pPing, int count, int iterations,
                   hr_time_t hrTimeout, const CNetHost& srcHost)
{
    return doMultiPing(pPing, count, iterations, hrTimeout, srcHost);
}
