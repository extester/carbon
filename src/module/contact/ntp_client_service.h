/*
 *  Carbon/Contact module
 *  Asynchronous NTP client service
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 20.06.2018 11:05:22
 *      Initial revision.
 */

#ifndef __CONTACT_NTP_CLIENT_SERVICE_H_INCLUDED__
#define __CONTACT_NTP_CLIENT_SERVICE_H_INCLUDED__

#include "shell/netaddr.h"

#include "carbon/event.h"
#include "carbon/event/eventloop.h"
#include "carbon/module.h"

#include "contact/ntp_client.h"

struct ntp_request_data_t {
	CNetHost 		ntpServer;
	CEventReceiver*	pReplyReceiver;
};

typedef CEventT<ntp_request_data_t, EV_NTP_CLIENT_REQUEST>	CEventNtpRequest;


struct ntp_reply_data_t {
	hr_time_t	hrTime;
	hr_time_t	hrTimestamp;
};

typedef CEventT<ntp_reply_data_t, EV_NTP_CLIENT_REPLY>	CEventNtpReply;

class CNtpClientService : public CModule, public CEventLoopThread, public CEventReceiver
{
    protected:
		CNtpClient			m_ntpClient;

    public:
		CNtpClientService(size_t nCount, hr_time_t hrTimeout);
        virtual ~CNtpClientService();

	public:
		virtual result_t init();
		virtual void terminate();

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);

		void doGetTime(const ntp_request_data_t* pData, seqnum_t nSessId);
};

#endif /* __CONTACT_NTP_CLIENT_SERVICE_H_INCLUDED__ */
