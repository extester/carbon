/*
 *  Carbon/Contact module
 *  NTP client 
 *
 *  Copyright (c) 2013-2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 26.08.2013
 *      Initial revision.
 */

#ifndef __CONTACT_NTP_CLIENT_H_INCLUDED__
#define __CONTACT_NTP_CLIENT_H_INCLUDED__

#include "shell/types.h"
#include "shell/hr_time.h"

struct ntptime {
    unsigned int coarse;
    unsigned int fine;
};

struct ntp_control {
    uint32_t time_of_send[2];
    char serv_addr[4];
};

class CNtpClient
{
    protected:
        size_t				m_nCount;       /* Max. iteration */
        hr_time_t			m_hrTimeout;	/* Reply awaiting timeout */

    public:
        CNtpClient(size_t nCount, hr_time_t hrTimeout);
        virtual ~CNtpClient();

    public:
        result_t synchronize(const char* strNtpServer);
		result_t getTime(const char* strNtpServer, hr_time_t* pTime, hr_time_t* pTimestamp);

    private:
        result_t setTime(struct ntptime *newtime) const;
        void getTime(uint32_t *time_coarse, uint32_t *time_fine) const;
        result_t sendPacket(int usd, uint32_t time_sent[2]) const;
        result_t stuffNetAddr(struct in_addr *p, const char *hostname);
        result_t setupReceive(int usd, unsigned int iface, short port);
        result_t setupTransmit(int usd, const char *host, short port, struct ntp_control *ntpc);
        result_t doSynchronize(int usd, struct ntp_control *ntpc);
		result_t doGetTime(int handle, struct ntp_control *ntpc, hr_time_t* pTime, hr_time_t* pTimestamp);
};

#endif /* __CONTACT_NTP_CLIENT_H_INCLUDED__ */
