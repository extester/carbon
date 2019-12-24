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
 *  Revision 1.0, 24.05.2015 20:35:07
 *      Initial revision.
 */

#ifndef __CONTACT_ICMP_H_INCLUDED__
#define __CONTACT_ICMP_H_INCLUDED__

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include "shell/hr_time.h"
#include "shell/netaddr.h"
#include "shell/socket.h"

#define ICMP_PACKET_TOTAL_LENGTH        (sizeof(struct ip)+sizeof(struct icmp))

#define ICMP_ITER_NOTSET                HR_0
#define ICMP_ITER_FAILED                ((hr_time_t)-1)

typedef struct
{
    CNetHost        dstHost;            /* [IN]     Destination host */
    hr_time_t*      arTimes;            /* {IN/OUT] Ping times per iteration (may be NULL) */
    uint32_t        cntSuccess;         /* [OUT]    Success iterations */
    uint32_t        cntFailed;          /* [OUT]    Failed iterations */

    /* -- internal use -- */
    int				stage;              /* Sending/Receiving stage */
    CSocketAsync    socket;             /* Communication socket */
    uint8_t         ioBuffer[ICMP_PACKET_TOTAL_LENGTH];
    size_t          ioLen;
    uint16_t        icmpIdent;
    uint16_t        icmpSeq;
} ping_request_t;

class CICmp
{
    private:
        char		        m_strTitle[64];
        uint8_t             m_templ[ICMP_PACKET_TOTAL_LENGTH];

        uint16_t            m_ipIdent;
        uint16_t            m_icmpIdent;
        uint16_t            m_icmpSeq;

        enum {
            echoIdle = 0,           /* no operation in progress */
            echoSending = 1,        /* sending echo stage */
            echoReceiving = 2,      /* receiving reply stage */
            echoDone = 3            /* completed */
        };

    public:
        CICmp(const char* strTitle);
        virtual ~CICmp();

        result_t ping(ping_request_t* pPing, int iterations, hr_time_t hrTimeout,
                            const CNetHost& srcHost);

        result_t multiPing(ping_request_t* pPing, int count, int iterations,
                            hr_time_t hrTimeout, const CNetHost& srcHost);

    private:
        boolean_t checkReply(ping_request_t* pPing, int index) const;
        result_t processActivity(ping_request_t* pPing, int index);

        result_t doSinglePing(int index, hr_time_t hrTimeout, ping_request_t* arPing, int count);
        result_t doMultiPing(ping_request_t* arPing, int count, int iterationCount,
                            hr_time_t hrTimeout, const CNetHost& srcHost);
};

#endif /* __CONTACT_ICMP_H_INCLUDED__ */
