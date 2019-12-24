/*
 *  Carbon/Contact module
 *  DNS resolution classes and functions
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 14.05.2015 20:51:31
 *      Initial revision.
 */
/*
 * Todo: add breaker
 */

#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <udns/udns.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/utils.h"

#include "contact/dns.h"


#define DNS_RESOLVE_RETRIES				3
#define DNS_RESOLVE_RETRY_TIMEOUT		2	/* Initial timeout, seconds */

#define DNS_RESULT_NOT_SET		        (-2)


/*******************************************************************************
 * CMultiDns class
 */

boolean_t CMultiDns::m_bInitialised = FALSE;

/*
 * Construct an object
 *
 *      strServer0          first DNS server ip (may be NULL)
 *      strServer1          second DNS server ip (may be NULL)
 *
 * Note:
 *      if both DNS servers are NULL than system DNS servers
 *      (from /etc/resolv.conf) are used.
 */
CMultiDns::CMultiDns(const char* strServer0, const char* strServer1) :
	m_nRetries(DNS_RESOLVE_RETRIES),
	m_nRetryTimeout(DNS_RESOLVE_RETRY_TIMEOUT),
	m_hBreaker(0)
{
	setServers(strServer0, strServer1);

	if ( !m_bInitialised )  {
		dns_init(NULL, 0);
		m_bInitialised = TRUE;
	}

    m_pCtx = dns_new(NULL);
    if ( !m_pCtx )  {
        log_error(L_GEN, "[dns] failed to create DNS context, possible out of memory\n");
    }
}

CMultiDns::~CMultiDns()
{
    if ( m_pCtx )  {
        dns_free(m_pCtx);
    }
}

/*
 * Set custom DNS servers
 *
 *      strServer0          first DNS server ip (may be NULL)
 *      strServer1          second DNS server ip (may be NULL)
 */
void CMultiDns::setServers(const char* strServer0, const char* strServer1)
{
    *m_strServer0 = '\0';
    *m_strServer1 = '\0';

    if ( strServer0 != 0 )  {
        copyString(m_strServer0, strServer0, sizeof(m_strServer0));
        if ( strServer1 != 0 )  {
            copyString(m_strServer1, strServer1, sizeof(m_strServer1));
        }
    }
    else {
        if ( strServer1 != 0 )  {
            copyString(m_strServer0, strServer1, sizeof(m_strServer0));
        }
    }
}

/*
 * Set resolving attempts and timeouts
 *
 *      nRetries            number of attempts
 *      nRepryTimeout       first resolve timeouts (see note below).
 *
 * Note: timeout are multiplies by 2 on every next attempts, for example:
 *
 *      attempt 1:  timeout:    2 sec
 *      attempt 2:  timeout:    2+4 = 6 sec
 *      attempt 3:  timeout:    2+4+8 = 14 sec
 */
void CMultiDns::setRetry(int nRetries, int nRetryTimeout)
{
    shell_assert(nRetries > 0);
    shell_assert(nRetryTimeout > 0);

    m_nRetries = nRetries;
    m_nRetryTimeout = nRetryTimeout;
}

/*
 * Convert single item (not containing '.') from win1251 encoding to punycode
 *
 * 		strSrc      source string
 * 		srcLen		source string length (not ended with '\0')
 * 		pOutBuf	    output buffer pointer [in/out]
 * 		pOutLen	    outbuf buffer length, bytes [in/out]
 *
 * 	Return:
 * 		ESUCCESS	success
 * 		E2BIG 		output buffer too short
 * 		EINVAL		conversion failed
 */
result_t CMultiDns::punycode_encode_item_win1251(const char* strSrc, size_t srcLen,
                                        char** pOutBuf, size_t* pOutLen) const
{
    enum punycode_status	status;
    punycode_uint			output_length;
    char*					ucs_buf;
    size_t					ucs_size;
    result_t 				nresult;

    /*
     * Convert to UNICODE
     */

    ucs_size = srcLen*4+4;
    ucs_buf = (char*)alloca(ucs_size);

    nresult = convertEncoding("WINDOWS-1251", "UCS-4LE", strSrc, srcLen, ucs_buf, &ucs_size);
    if ( nresult != ESUCCESS )  {
        UNALIGNED_MEMCPY(ucs_buf, strSrc, srcLen);
        ucs_buf[srcLen] = '\0';
        log_debug(L_GEN, "[dns] failed to convert to unicode, domain %s, result=%d\n",
               ucs_buf, nresult);
        if ( nresult != E2BIG )  {
            nresult = EINVAL;
        }

        return nresult;
    }

    /*
     * Punycode encoding
     */

    shell_assert(ucs_size == (srcLen*4));

    if ( (*pOutLen) <= 4 )  {
        UNALIGNED_MEMCPY(ucs_buf, strSrc, srcLen);
        ucs_buf[srcLen] = '\0';
        log_debug(L_GEN, "[dns] outbuf buffer too short #2, domain %s, result=%d\n", ucs_buf);
        return E2BIG;
    }

    UNALIGNED_MEMCPY(*pOutBuf, "xn--", 4);
    *pOutLen -= 4;
    *pOutBuf += 4;

    output_length = (punycode_uint)*pOutLen;
    status = punycode_encode((punycode_uint)ucs_size/4, (const punycode_uint*)ucs_buf, NULL,
                             &output_length, *pOutBuf);
    if ( status == punycode_success )  {
        *pOutLen -= output_length;
        *pOutBuf += output_length;
    }
    else  {
        UNALIGNED_MEMCPY(ucs_buf, strSrc, srcLen);
        ucs_buf[srcLen] = '\0';

        switch(status)  {
            case punycode_bad_input:
                /* Input is invalid. */
                log_debug(L_GEN, "[dns] punycode_encode() failed, domain %s, result=punycode_bad_input\n",
                       ucs_buf);
                nresult = EINVAL;
                break;

            case punycode_big_output:
                /* Output would exceed the space provided. */
                log_debug(L_GEN, "[dns] punycode_encode() failed, domain %s, result=punycode_big_output\n",
                       ucs_buf);
                nresult = E2BIG;
                break;

            case punycode_overflow:
                /* Input needs wider integers to process.  */
                log_debug(L_GEN, "[dns] punycode_encode() failed, domain %s, result=punycode_overflow\n",
                       ucs_buf);
                nresult = EINVAL;
                break;

            default:
                nresult = EINVAL;
                log_debug(L_GEN, "[dns] punycode_encode() failed, domain %s, wrong result=%d\n",
                       ucs_buf, status);
                break;
        }
    }

    return nresult;
}

/*
 * Convert domain name to punycode
 *
 * 		strDst 		    output buffer
 * 		dstSize	        output buffer size, bytes
 * 		strSrc 		    source win1251 string
 *
 * Return:
 * 		ESUCCESS		success
 * 		E2BIG			destination buffer too short
 * 		EINVAL			conversation failed
 */
result_t CMultiDns::dns_punycode_encode_win1251(char* strDst, size_t dstSize,
                                                const char* strSrc) const
{
    const char				*s, *item;
    boolean_t				bEncode;
    char					*outBuf;
    size_t					itemLen, outLen;
    result_t				nresult = ESUCCESS;

    /*
     * Check encoding
     *
     */
    s = strSrc;
    bEncode = FALSE;
    while ( *s != '\0' && !bEncode )  {
        bEncode = ((*s)&0x80) != 0;  //!isascii(*s);
        s++;
    }

    if ( !bEncode )  {
        if ( _tstrlen(strSrc) > (dstSize-1) )  {
            log_debug(L_GEN, "[dns] destination buffer to short #1, domain %s\n", strSrc);
            nresult = E2BIG;
        }
        else  {
            _tstrcpy(strDst, strSrc);
        }

        return nresult;
    }

    /*
     * Encode separated domain items: "verinet" "ru"
     */
    s = strSrc;
    outBuf = strDst;
    outLen = dstSize-1;

    while ( *s != '\0' )  {
        item = _tstrchr(s, '.');
        if ( item != NULL )  {
            itemLen = A(item) - A(s);
            item = s;
        }
        else  {
            itemLen = _tstrlen(s);
            item = s;
        }

        if ( itemLen > 0 )  {
            /*
             * Convert item
             */
            nresult = punycode_encode_item_win1251(item, itemLen, &outBuf, &outLen);
            if ( nresult != ESUCCESS )  {
                break;
            }
        }

        s = item + itemLen;
        if ( *s == '.' ) {
            if ( outLen < 1 )  {
                log_debug(L_GEN, "[dns] destination buffer to short #2, domain %s\n", strSrc);
                nresult = E2BIG;
                break;
            }

            *outBuf = '.';
            outBuf ++;
            outLen --;
            s++;
        }
    }

    if ( nresult == ESUCCESS )  {
        *outBuf = '\0';
        /*log_info(L_GEN, "[dns] conversion done: '%s' => '%s'\n", strSrc, strDst);*/
    }

    return nresult;
}


/*
 * Convert UDNS error code to UNIX error code
 *
 * 		pDns 		dns domain
 * 		dnsError	udns error code
 *
 * Return: unix error code
 */
result_t CMultiDns::dns2unixError(dns_request_t* pDns, int dnsError) const
{
    result_t    nresult;

    switch ( dnsError )  {
        case DNS_E_TEMPFAIL:
            nresult = EBADF;
            break;

        case DNS_E_PROTOCOL:
        case DNS_E_NODATA:
            nresult = EFAULT;
            log_debug(L_GEN, "[dns] dns server error %s resolving %s, returning EFAULT(%d)\n",
                   dns_strerror(dnsError), pDns->strDomain, nresult);
            break;

        case DNS_E_NXDOMAIN:
            nresult = ENOENT;
            log_debug(L_GEN, "[dns] no domain %s found\n", pDns->strDomain);
            break;

        case DNS_E_NOMEM:
            nresult = ENOMEM;
            log_debug(L_GEN, "[dns] out of memory error\n");
            break;

        case DNS_E_BADQUERY:
            nresult = EINVAL;
            log_debug(L_GEN, "[dns] bad parameters of domain %s\n", pDns->strDomain);
            break;

        default:
            nresult = EINVAL;
            log_debug(L_GEN, "[dns] invalid UDNS error code %d\n", dnsError);
            break;
    }

    return nresult;
}

/*
 * Resolve callback
 */
void CMultiDns::callback(struct dns_rr_a4* result, dns_request_t* pDns)
{
    int 			status;

    shell_assert(pDns->nresult == DNS_RESULT_NOT_SET);

    status = dns_status(m_pCtx);
    if ( status == DNS_E_NOERROR && result != NULL )  {
        if ( result->dnsa4_nrr > 0 )  {
            /* Use a first address */
            pDns->inaddr = result->dnsa4_addr[0];
            pDns->nresult = ESUCCESS;

            /* Dump debug info */
#if 0
			{
				char	s[64];
				int		ii;

				log_dump(">> Dump resolved domain '%s'\n", pDns->strDomain);
				log_dump("   original query name: %s\n", result->dnsa4_qname);
				log_dump("   canonical name: %s\n", result->dnsa4_cname);
				log_dump("   TTL: %d\n", result->dnsa4_ttl);

				for(ii=0; ii<result->dnsa4_nrr; ii++)  {
					if ( inaddr2ipaddr(result->dnsa4_addr[ii], s) != ESUCCESS )  {
						_tstrcpy(s, "**error**");
					}

					log_dump("   address%d: %s\n", ii, s);
				}
			}
#endif
        }
        else  {
            log_error(L_GEN, "[dns] domain %s resolved, but no addresses found\n",
                   pDns->strDomain);
            pDns->nresult = EFAULT;
        }
    }

    if ( result )  {
        free(result);
    }

    if ( pDns->nresult == DNS_RESULT_NOT_SET )  {
        pDns->nresult = status == DNS_E_NOERROR ? EFAULT : dns2unixError(pDns, status);
        /*log_debug(L_GEN, "[dns] domain: %s, result: %s(%d)\n", pDns->strDomain,
                        strerror(pDns->nresult), pDns->nresult);*/
    }
}

/*
 * Parallel DNS multi-resolving
 *
 *      arDns           DNS resolve request array
 *      count           array elements
 *      hrTimeout       maximum resolve timeout
 *
 * Result:
 *      ESUCCESS        resolved, see individual result codes
 *      ENOMEM          out of memory
 *      ETIMEDOUT       failed to resolve any request
 *      EINTR           interrupted by signal
 *      ...
 */
result_t CMultiDns::resolve(dns_request_t* arDns, int count, hr_time_t hrTimeout)
{
    struct pollfd	pfd;
    int             i, msTimeout, pending;
    result_t        nresult;

    /*
     * Sanity checks
     */
    shell_assert(count > 0);
    if ( count < 1 )  {
        return ESUCCESS;
    }

    if ( !m_pCtx ) {
        return ENOMEM;
    }

    msTimeout = (int)HR_TIME_TO_MILLISECONDS(hrTimeout);
    if ( msTimeout < 1 )  {
        return ETIMEDOUT;
    }

    /*
     * Setup resolve parameters
     */
    dns_reset(m_pCtx);

    /* This sets default dns servers from /etc/resolv.conf */
    dns_init(m_pCtx, FALSE);

    if ( *m_strServer0 != '\0' )  {
        dns_reset(m_pCtx);

        dns_add_serv(m_pCtx, m_strServer0);
        if ( *m_strServer1 != '\0' )  {
            dns_add_serv(m_pCtx, m_strServer1);
        }
    }

    dns_set_opt(m_pCtx, DNS_OPT_NTRIES, m_nRetries);
    dns_set_opt(m_pCtx, DNS_OPT_TIMEOUT, m_nRetryTimeout);

    pfd.fd = dns_open(m_pCtx);
    if ( pfd.fd < 0 )  {
        log_error(L_GEN, "[dns] failed to open UDNS context, possible resource shortage\n");
        return ENOMEM;
    }

    /*
     * Submit queries
     */
    nresult = ESUCCESS;
    pending = 0;

    for(i=0; i<count; i++) {
        dns_request_t*      pDns = &arDns[i];
        struct dns_query*	q;
        char			    dnbuf[DNS_MAXDN+1];
        result_t            nr;

        nr = dns_punycode_encode_win1251(dnbuf, sizeof(dnbuf), pDns->strDomain);
        if ( nr != ESUCCESS )  {
            pDns->nresult = nr;
            log_debug(L_GEN, "[dns] failed to punycode domain name %s, result: %d\n",
                   pDns->strDomain, nr);
            continue;
        }

        pDns->pParent = this;
        q = dns_submit_a4(m_pCtx, dnbuf, 0, callbackST, pDns);
        if ( q )  {
            pDns->nresult = DNS_RESULT_NOT_SET;
            pending ++;
        }
        else  {
            pDns->nresult = dns2unixError(pDns, dns_status(m_pCtx));
            log_debug(L_GEN, "[dns] failed to submit DNS query, domain '%s', dns result: %d, result %d\n",
					  pDns->strDomain, dns_status(m_pCtx), pDns->nresult);
        }

    }

    /*
     * Wait for completion
     */
    if ( pending > 0 )  {
        int 	    nsec, n;
        hr_time_t   hrStart, hrElapsed;

        nresult = ESUCCESS;
        hrStart = hr_time_now();

        while( (nsec=dns_timeouts(m_pCtx, -1, 0)) > 0 )  {
            hrElapsed = hr_time_get_elapsed(hrStart);

            if ( hrElapsed > hrTimeout ||
                 (msTimeout=(int)HR_TIME_TO_MILLISECONDS(hrTimeout-hrElapsed)) < 1 )
            {
                nresult = ETIMEDOUT;
                break;
            }

            pfd.events = POLLIN;
            pfd.revents = 0;
            n = poll(&pfd, 1, MIN(msTimeout, nsec*1000));

            if ( n == 0 )  {
                continue;
            }

            if ( n < 0 )  {
                nresult = errno;
                log_error(L_GEN, "[dns] awaiting poll() failed, result: %d\n", nresult);
                break;
            }

            dns_ioevent(m_pCtx, 0);

            if ( pfd.revents&(POLLERR|POLLHUP|POLLNVAL) )  {
                log_debug(L_GEN, "[dns] awaiting poll() returns error flags 0x%X, "
                        "continue awaiting\n", pfd.revents&(POLLERR|POLLHUP|POLLNVAL));
                usleep(500000); /* 0.5 sec */
            }
        }
    }

    /*
     * Finish up
     */
    dns_close(m_pCtx);

    for(i=0; i<count; i++)  {
        if ( arDns[i].nresult == DNS_RESULT_NOT_SET )  {
            if ( nresult == ESUCCESS )  {
                log_error(L_GEN, "[dns] resolved ok, but domain %s has no result code\n",
                       arDns[i].strDomain);
            }

            arDns[i].nresult = nresult != ESUCCESS ? nresult : EINVAL;
        }
    }

    return nresult;
}


/*******************************************************************************
 * DNS utility functions
 */

result_t __singleLookup(CMultiDns& resolver, const char* strDomain,
                    hr_time_t hrTimeout, struct in_addr* pInaddr/*out*/)
{
    struct in_addr  inaddr;
    dns_request_t   arDns[1];
    int             retVal;
    result_t        nresult;

    shell_assert(strDomain);
    shell_assert(pInaddr);

    /* Check for "n.n.n.n */
    retVal = inet_aton(strDomain, &inaddr);
    if ( retVal != 0 ) {
        *pInaddr = inaddr;
        return ESUCCESS;
    }

    copyString(arDns[0].strDomain, strDomain, sizeof(arDns[0].strDomain));

    nresult = resolver.resolve(arDns, 1, hrTimeout);
    if ( nresult == ESUCCESS ) {
        if ( arDns[0].nresult == ESUCCESS ) {
            *pInaddr = arDns[0].inaddr;
        }
        else {
            nresult = arDns[0].nresult;
        }
    }

    return nresult;
}

result_t dnsLookup(const char* strDomain, hr_time_t hrTimeout,
                          const char* strDnsServer0, const char* strDnsServe1,
                          struct in_addr* pInaddr/*out*/) {


    CMultiDns resolver(strDnsServer0, strDnsServe1);
    return __singleLookup(resolver, strDomain, hrTimeout, pInaddr);
}

result_t dnsLookup(const char* strDomain, hr_time_t hrTimeout, struct in_addr* pInaddr/*out*/)
{
    CMultiDns resolver;
    return __singleLookup(resolver, strDomain, hrTimeout, pInaddr);
}
