/*
 *  Carbon/Contact module
 *  DNS related classes and functions
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 14.05.2015 20:35:24
 *      Initial revision.
 */

#ifndef __CONTACT_DNS_H_INCLUDED__
#define __CONTACT_DNS_H_INCLUDED__

#include <sys/socket.h>
#include <netinet/in.h>

#include "shell/shell.h"
#include "shell/hr_time.h"

#include "contact/punycode.h"

class CMultiDns;
struct dns_rr_a4;
struct dns_ctx;

typedef struct {
	char				strDomain[128];     /* Domain to resolve [IN] */

	struct in_addr		inaddr;             /* Resolved IP [OUT] */
	result_t			nresult;            /* Operation result [OUT] */

    /* --- internal use only */
    CMultiDns*          pParent;            /* Parent object */
} dns_request_t;

/*
 * Class implements parallel multi-domain resolution
 */
class CMultiDns
{
	private:
		static boolean_t	    m_bInitialised;     /* One-time UDNS library initialisation */

	protected:
		int			            m_nRetries;
		int			            m_nRetryTimeout;

		char		            m_strServer0[128];
		char		            m_strServer1[128];

		int			            m_hBreaker;
        struct dns_ctx*		    m_pCtx;

	public:
		CMultiDns(const char* strServer0 = 0, const char* strServer1 = 0);
		/*CMultiDns(int hBreaker, const char* strServer0 = 0, const char* strServer1 = 0);*/
		virtual ~CMultiDns();

	public:
		result_t resolve(dns_request_t* arDns, int count, hr_time_t hrTimeout);
		void setRetry(int nRetries, int nRetryTimeout);

	private:
		void setServers(const char* strServer0, const char* strServer1);
        result_t dns_punycode_encode_win1251(char* strDst, size_t dstSize, const char* strSrc) const;
        result_t punycode_encode_item_win1251(const char* strSrc, size_t srcLen,
										    char** pOutBuf, size_t* pOutLen) const;

        result_t dns2unixError(dns_request_t* pDns, int dnsError) const;
        static void callbackST(struct dns_ctx* pCtx, struct dns_rr_a4* result, void* pData) {
            dns_request_t*  pDns = static_cast<dns_request_t*>(pData);

            shell_assert(pDns->pParent);
            pDns->pParent->callback(result, pDns);
        }
        void callback(struct dns_rr_a4* result, dns_request_t* pDns);
};

extern result_t __singleLookup(CMultiDns& resolver, const char* strDomain,
						hr_time_t hrTimeout, struct in_addr* pInaddr/*out*/);

extern result_t dnsLookup(const char* strDomain, hr_time_t hrTimeout,
						const char* strDnsServer0, const char* strDnsServe1,
						struct in_addr* pInaddr/*out*/);

extern result_t dnsLookup(const char* strDomain, hr_time_t hrTimeout,
						struct in_addr* pInaddr/*out*/);


#endif /* __CONTACT_DNS_H_INCLUDED__ */
