/*
 *  Carbon/Contact module
 *  NTP client
 *
 *  Copyright (c) 2013-2019 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 26.08.2013
 *      Initial revision.
 *
 *  Revision 1.1, 21.02.2019 11:52:39
 *  	Fixed bug in doGetTime() and doSynchronize():
 *  	added missing sa_xmit_len = sizeof(sa_xmit);
 */

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>     
#include <arpa/inet.h>

#include "shell/shell.h"
#include "shell/logger.h"
#include "shell/hr_time.h"

#include "contact/ntp_client.h"

#define JAN_1970            0x83aa7e80      /* 2208988800 1970 - 1900 in seconds */
#define NTP_PORT            123

/* How to multiply by 4294.967296 quickly (and not quite exactly)
 * without using floating point or greater than 32-bit integers.
 * If you want to fix the last 12 microseconds of error, add in
 * (2911*(x))>>28)
 */
#define NTPFRAC(x) ( 4294*(x) + ( (1981*(x))>>11 ) )

/* The reverse of the above, needed if we want to set our microsecond
 * clock (via clock_settime) based on the incoming time in NTP format.
 * Basically exact.
 */
#define USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )


CNtpClient::CNtpClient(size_t nCount, hr_time_t hrTimeout) :
    m_nCount(nCount),
    m_hrTimeout(hrTimeout)
{
}

CNtpClient::~CNtpClient()
{
}

result_t CNtpClient::setTime(struct ntptime *newtime) const
{
    /*
     * POSIX 1003.1-2001 way to set the system clock
     */
    struct timespec tv_set;
//    time_t  t1 = time(NULL), t2;
    result_t nresult = ESUCCESS;

    /* it would be even better to subtract half the slop */
    tv_set.tv_sec  = newtime->coarse - JAN_1970;

    /* divide xmttime.fine by 4294.967296 */
    tv_set.tv_nsec = USEC(newtime->fine)*1000;
    if ( clock_settime(CLOCK_REALTIME, &tv_set) < 0 ) {
        nresult = errno;
        log_error(L_GEN, "[ntp_client] failed to set system time, result=%d\n", nresult);
    }
    /*else  {
        t2 = time(NULL);
        log_debuginfo(L_GEN, "[ntp_client] time successfully synchronized, delta=%d sec\n", t2-t1);
    }*/

    return nresult;
}

void CNtpClient::getTime(uint32_t *time_coarse, uint32_t *time_fine) const
{
    /*
     * POSIX 1003.1-2001 way to get the system time
     */
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    *time_coarse = now.tv_sec + JAN_1970;
    *time_fine   = NTPFRAC(now.tv_nsec/1000);
}

result_t CNtpClient::sendPacket(int usd, uint32_t time_sent[2]) const
{
    uint32_t	data[12];
    int 		retVal;
	result_t 	nresult = ESUCCESS;
    
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4
#define PREC -6

    memset(data,0,sizeof data);
    data[0] = htonl (
        ( LI << 30 ) | ( VN << 27 ) | ( MODE << 24 ) |
        ( STRATUM << 16) | ( POLL << 8 ) | ( PREC & 0xff ) );
    data[1] = htonl(1<<16);  /* Root Delay (seconds) */
    data[2] = htonl(1<<16);  /* Root Dispersion (seconds) */
    getTime(time_sent, time_sent+1);
    data[10] = htonl(time_sent[0]); /* Transmit Timestamp coarse */
    data[11] = htonl(time_sent[1]); /* Transmit Timestamp fine   */

    retVal = send(usd,data,48,0);
    if ( retVal != 48 )  {
        nresult = errno;
        log_error(L_GEN, "[ntp_client] failed to send request to the NTP server, "
                "sent only %d bytes, result=%d\n", retVal, nresult);
    }

    return nresult;
}

result_t CNtpClient::stuffNetAddr(struct in_addr *p, const char *hostname)
{
    struct hostent *ntpserver;

    ntpserver=gethostbyname(hostname);
    if (ntpserver == NULL) {
        log_error(L_GEN, "[ntp_client] gethostbyname() failed, server='%s', h_errno=%d\n",
                        hostname, h_errno);
        return ENOENT;
    }
    
    if (ntpserver->h_length != 4) {
        /* IPv4 only, until I get a chance to test IPv6 */
    	log_error(L_GEN, "[ntp_client] gethostbyname() is not IP4, server: '%s'\n", hostname);
        return EINVAL;
    }
    memcpy(&(p->s_addr),ntpserver->h_addr_list[0],4);
    return ESUCCESS;
}

result_t CNtpClient::setupReceive(int usd, unsigned int iface, short port)
{
    struct sockaddr_in sa_rcvr;
    result_t nresult = ESUCCESS;
    
    memset(&sa_rcvr,0,sizeof sa_rcvr);
    sa_rcvr.sin_family=AF_INET;
    sa_rcvr.sin_addr.s_addr=htonl(iface);
    sa_rcvr.sin_port=htons(port);
    if(bind(usd,(struct sockaddr *) &sa_rcvr,sizeof sa_rcvr) == -1) {
        nresult = errno;
        log_error(L_GEN, "[ntp_client] could not bind to UDP port %d, result %d\n",
                        port, nresult);
    }

    return nresult;
}

result_t CNtpClient::setupTransmit(int usd, const char *host, short port, struct ntp_control *ntpc)
{
    struct sockaddr_in sa_dest;
    result_t nresult = ESUCCESS;
    
    memset(&sa_dest,0,sizeof sa_dest);
    sa_dest.sin_family=AF_INET;
    stuffNetAddr(&(sa_dest.sin_addr),host);
    memcpy(ntpc->serv_addr,&(sa_dest.sin_addr),4); /* XXX asumes IPv4 */
    sa_dest.sin_port=htons(port);
    if (connect(usd,(struct sockaddr *)&sa_dest,sizeof sa_dest)==-1) {
        nresult = errno;
        log_error(L_GEN, "[ntp_client] connect() failed, nresult=%d\n", nresult);
    }

    return nresult;
}

result_t CNtpClient::doSynchronize(int usd, struct ntp_control *ntpc)
{
    fd_set fds;
    struct sockaddr sa_xmit;
    struct sockaddr_in *sa_in;
    struct ntptime orgtime, xmttime;
    socklen_t sa_xmit_len;
    struct timeval to;
    static uint32_t incoming_word[325];
#define incoming ((char *) incoming_word)
#define sizeof_incoming (sizeof incoming_word)
    result_t     nresult = ESUCCESS;
	int		pack_len, n, stratum;
    size_t     count = m_nCount;
    hr_time_t now = hr_time_now();

    while ( count-- > 0 )  {
        if ( hr_time_get_elapsed(now) >= (hr_time_t)(m_nCount*m_hrTimeout) )  {
            nresult = ETIMEDOUT;
            break;
        }
        
        nresult = sendPacket(usd, ntpc->time_of_send);
        if ( nresult != ESUCCESS )  {
            if ( nresult == EINTR )  {
                break;
            }
            if ( hr_sleep(HR_2SEC) != ESUCCESS )  {
				nresult = EINTR;
                break;
            }                
            continue;
        }            

        FD_ZERO(&fds);
        FD_SET(usd,&fds);
        
        to.tv_sec = HR_TIME_TO_SECONDS(m_hrTimeout);
        to.tv_usec = 0;

        n = select(usd+1, &fds, NULL, NULL, &to);  /* Wait on read or error */
        if ((n!=1)||(!FD_ISSET(usd,&fds))) {
            if (n<0) {
                nresult = errno;
                if ( nresult == EINTR )  {
                    break;
                }
            }

            if ( n == 0 )  {
                nresult = ETIMEDOUT;
            }                                
            continue;
        }

		sa_xmit_len = sizeof(sa_xmit);
		pack_len=recvfrom(usd,incoming,sizeof_incoming,0,
                          &sa_xmit,&sa_xmit_len);
        if (pack_len<0) {
            nresult = errno;
            continue;
        } else if (pack_len>0 && (unsigned)pack_len<sizeof_incoming){
            /*
             * Check source
             */
            sa_in=(struct sockaddr_in *)&sa_xmit;
            if (NTP_PORT != ntohs(sa_in->sin_port)) {
                /* Uninterested packet */
                count++;
                continue;
            }

            orgtime.coarse = ntohl(incoming_word[6]);
            orgtime.fine   = ntohl(incoming_word[7]);
            
            xmttime.coarse = ntohl(incoming_word[10]);
            xmttime.fine   = ntohl(incoming_word[11]);

            stratum = (ntohl(incoming_word[0]) >> 16) & 0xff;

            if ( orgtime.coarse != ntpc->time_of_send[0] ||
                    orgtime.fine != ntpc->time_of_send[1] )
            {
                nresult = EINVAL;
                count++;
                continue;
            }
            
            if ( xmttime.coarse == 0 && xmttime.fine == 0 ) {
                nresult = EINVAL;
				count++;
                log_error(L_GEN, "[ntp_client] zero reply time\n");
                continue;
            }

            if ( stratum == 0 )  {
                nresult = EINVAL;
                log_error(L_GEN, "[ntp_client] zero stratum\n");
				count++;
                continue;
            }

            nresult = setTime(&xmttime);
            if ( nresult == ESUCCESS )  {
                break;
            }                
        }
        else  {
            count++;
        }            
    }

    if ( nresult == ETIMEDOUT )  {
    	log_error(L_GEN, "[ntp_client] no response from NTP server\n");
    }        

    return nresult;
}

/*
 * Process time synchronization
 *
 * Return: ESUCCESS, ...
 */
result_t CNtpClient::synchronize(const char* strNtpServer)
{
    struct ntp_control  ntpc;
    int                 usd;
    result_t			nresult;

    if ( strNtpServer == 0 || *strNtpServer == '\0' )  {
    	log_error(L_GEN, "[ntp_client] missing NTP server ip\n");
        return EINVAL;
    }        
    
    usd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if ( usd < 0 )  {
        nresult = errno;
        log_error(L_GEN, "[ntp_client] failed to create socket, result=%d\n", nresult);
        return nresult;
    }
    
    nresult = setupReceive(usd, INADDR_ANY, 0/*local port, kernel chooses*/);
    if ( nresult != ESUCCESS )  {
        close(usd);
        return nresult;
    }

    nresult = setupTransmit(usd, strNtpServer, NTP_PORT, &ntpc);
    if ( nresult != ESUCCESS )  {
        close(usd);
        return nresult;
    }

    nresult = doSynchronize(usd, &ntpc);
    close(usd);

    return nresult;
}

result_t CNtpClient::doGetTime(int handle, struct ntp_control *ntpc, hr_time_t* pTime, hr_time_t* pTimestamp)
{
	fd_set fds;
	struct sockaddr sa_xmit;
	struct sockaddr_in *sa_in;
	struct ntptime orgtime, xmttime;
	socklen_t sa_xmit_len;
	struct timeval to;
	static uint32_t incoming_word[325];
#define incoming ((char *) incoming_word)
#define sizeof_incoming (sizeof incoming_word)
	result_t     nresult = ESUCCESS;
	int		pack_len, n, stratum;
	size_t     count = m_nCount;
	hr_time_t now = hr_time_now();

	while ( count-- > 0 )  {
		if ( hr_time_get_elapsed(now) >= (hr_time_t)(m_nCount*m_hrTimeout) )  {
			nresult = ETIMEDOUT;
			break;
		}

		nresult = sendPacket(handle, ntpc->time_of_send);
		if ( nresult != ESUCCESS )  {
			if ( nresult == EINTR )  {
				break;
			}
			if ( hr_sleep(HR_2SEC) != ESUCCESS )  {
				nresult = EINTR;
				break;
			}
			continue;
		}

		FD_ZERO(&fds);
		FD_SET(handle, &fds);

		to.tv_sec = HR_TIME_TO_SECONDS(m_hrTimeout);
		to.tv_usec = 0;

		n = select(handle+1, &fds, NULL, NULL, &to);  /* Wait on read or error */
		if ((n!=1)||(!FD_ISSET(handle, &fds))) {
			if (n<0) {
				nresult = errno;
				if ( nresult == EINTR )  {
					break;
				}
			}

			if ( n == 0 )  {
				nresult = ETIMEDOUT;
			}
			continue;
		}

		sa_xmit_len = sizeof(sa_xmit);
		pack_len=recvfrom(handle,incoming,sizeof_incoming,0,
						  &sa_xmit,&sa_xmit_len);
		if (pack_len<0) {
			nresult = errno;
			continue;
		} else if (pack_len>0 && (unsigned)pack_len<sizeof_incoming){
			/*
			 * Check source
			 */
			sa_in=(struct sockaddr_in *)&sa_xmit;
			if (NTP_PORT != ntohs(sa_in->sin_port)) {
				/* Uninterested packet */
				count++;
				continue;
			}

			orgtime.coarse = ntohl(incoming_word[6]);
			orgtime.fine   = ntohl(incoming_word[7]);

			xmttime.coarse = ntohl(incoming_word[10]);
			xmttime.fine   = ntohl(incoming_word[11]);

			stratum = (ntohl(incoming_word[0]) >> 16) & 0xff;

			if ( orgtime.coarse != ntpc->time_of_send[0] ||
				 orgtime.fine != ntpc->time_of_send[1] )
			{
				nresult = EINVAL;
				count++;
				continue;
			}

			if ( xmttime.coarse == 0 && xmttime.fine == 0 ) {
				nresult = EINVAL;
				count++;
				log_error(L_GEN, "[ntp_client] zero reply time\n");
				continue;
			}

			if ( stratum == 0 )  {
				nresult = EINVAL;
				count++;
				log_error(L_GEN, "[ntp_client] zero stratum\n");
				continue;
			}

			/* Success */
			*pTimestamp = hr_time_now();
			*pTime = SECONDS_TO_HR_TIME(xmttime.coarse - JAN_1970) +
						MICROSECONDS_TO_HR_TIME(USEC(xmttime.fine));
			nresult = ESUCCESS;
			break;
		}
		else  {
			count++;
		}
	}

	if ( nresult == ETIMEDOUT )  {
		log_error(L_GEN, "[ntp_client] no response from NTP server\n");
	}

	return nresult;
}

result_t CNtpClient::getTime(const char* strNtpServer, hr_time_t* pTime, hr_time_t* pTimestamp)
{
	struct ntp_control  ntpc;
	int                 handle;
	result_t			nresult;

	if ( strNtpServer == 0 || *strNtpServer == '\0' )  {
		log_error(L_GEN, "[ntp_client] missing NTP server ip\n");
		return EINVAL;
	}

	handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if ( handle < 0 )  {
		nresult = errno;
		log_error(L_GEN, "[ntp_client] failed to create socket, result=%d\n", nresult);
		return nresult;
	}

	nresult = setupReceive(handle, INADDR_ANY, 0/*local port, kernel chooses*/);
	if ( nresult != ESUCCESS )  {
		close(handle);
		return nresult;
	}

	nresult = setupTransmit(handle, strNtpServer, NTP_PORT, &ntpc);
	if ( nresult != ESUCCESS )  {
		close(handle);
		return nresult;
	}

	nresult = doGetTime(handle, &ntpc, pTime, pTimestamp);
	close(handle);

	return nresult;

}
