/*
 *  Shell library
 *  Network (IP V4) destination address and port
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 20.04.2015 23:22:43
 *      Initial revision.
 *
 *  Revision 1.1, 05.29.2015 15:48:02
 *  	Added CNetHost class
 *
 *  Revision 1.2, 24.08.2017 17:00:11
 *  	Renames strHost() to c_str()
 */

#ifndef __SHELL_NETADDR_H_INCLUDED__
#define __SHELL_NETADDR_H_INCLUDED__

#include <netinet/in.h>

#include "shell/shell.h"

#define IPV4_ADDR_LEN	16

typedef uint32_t		ip_addr_t;		/* Network byteorder */
typedef uint16_t		ip_port_t;		/* Network byteorder */

#define NETHOST_NULL	(CNetHost())

/*
 * Class represents a single IP V4 address
 */
class CNetHost
{
	protected:
		ip_addr_t		m_ip;			/* IP address, 32-bit network byte order value */
		boolean_t		m_bValid;		/* TRUE: Ip address is valid, FALSE: invalid */
		mutable char	m_strBuf[IPV4_ADDR_LEN+16];

	public:
		CNetHost() : m_ip(0), m_bValid(FALSE)  {}
		CNetHost(const CNetHost& host) { *this = host; }
		CNetHost(struct in_addr inaddr) { setHost(inaddr); }
		CNetHost(ip_addr_t nIpAddr) : m_ip(nIpAddr), m_bValid(TRUE) {}
		CNetHost(const char* strHost) { setHost(strHost); }

		virtual ~CNetHost() {}

	public:
		CNetHost& operator=(const CNetHost& host) {
			m_ip = host.m_ip;
			m_bValid = host.m_bValid;
			return *this;
		}

		CNetHost& operator=(const char* strHost) {
			setHost(strHost);
			return *this;
		}

		boolean_t operator==(const CNetHost& host) const {
			return m_ip == host.m_ip && m_bValid && host.m_bValid;
		}

		boolean_t operator!=(const CNetHost& host) const {
			return m_ip != host.m_ip || !m_bValid || !host.m_bValid;
		}

		virtual explicit operator const char*() const {
			return cs();
		}

		operator struct in_addr() const {
			struct in_addr inaddr;
			inaddr.s_addr = m_ip;
			return inaddr;
		}

		operator ip_addr_t() const {
			return m_ip;
		}

		void setHost(ip_addr_t nIpAddr) {
			m_ip = nIpAddr;
			m_bValid = TRUE;
		}

		void setHost(struct in_addr inaddr) {
			m_ip = inaddr.s_addr;
			m_bValid = TRUE;
		}

		boolean_t setHost(const char* strHost);

		const char* getHost() { return CNetHost::cs(); }

		virtual const char* cs() const {
			if ( isValid() ) {
				strHost_r(m_strBuf, sizeof(m_strBuf));
				return m_strBuf;
			}
			return "<not-set>";
		}

		virtual const char* c_str() const {
			return cs();
		}

		virtual void strHost_r(char* strHost, size_t length) const;

		virtual boolean_t isValid() const {
			return m_bValid;
		}
};

/*******************************************************************************
 * CNetAddr
 *
 */

#define NETADDR_NULL		(CNetAddr())

class CNetAddr : public CNetHost
{
	protected:
		ip_port_t		m_port;

	public:
		CNetAddr() : CNetHost(), m_port(0) {}
		CNetAddr(const CNetAddr& src) : CNetHost() { *this = src; }
		CNetAddr(ip_addr_t nIpAddr, ip_port_t nPort) : CNetHost(nIpAddr), m_port(htons(nPort)) {}
		CNetAddr(struct in_addr inaddr, ip_port_t nPort) : CNetHost(inaddr.s_addr), m_port(htons(nPort)) {}
		CNetAddr(const char* strIp, ip_port_t nPort) : CNetHost(), m_port(htons(nPort)) { setHost(strIp); }
		CNetAddr(const CNetHost& host, ip_port_t nPort = 0) : CNetHost(host), m_port(htons(nPort)) {}

		virtual ~CNetAddr() {}

	public:
		boolean_t operator==(const CNetAddr& addr) const {
			return m_ip == addr.m_ip && m_port == addr.m_port &&
				m_bValid && addr.m_bValid;
		}

		boolean_t operator!=(const CNetAddr& addr) const {
			return m_ip != addr.m_ip || m_port != addr.m_port ||
				!m_bValid || !addr.m_bValid;
		}


		virtual explicit operator const char*() const {
			return cs();
		}

		operator ip_port_t() const {
			return ntohs(m_port);
		}

		uint16_t getPort() const { return ntohs(m_port); }

		void setAddr(ip_addr_t nIpAddr, ip_port_t nPort) {
			m_ip = nIpAddr; m_port = htons(nPort); m_bValid = TRUE;
		}

		void setAddr(struct in_addr inaddr, ip_port_t nPort) {
			m_ip = inaddr.s_addr; m_port = htons(nPort); m_bValid = TRUE;
		}

		void setAddr(const char* strIp, ip_port_t nPort) {
			setHost(strIp); m_port = htons(nPort);
		}

		virtual void strAddr_r(char* strAddr, size_t length) const;

		virtual const char* cs() const {
			if ( isValid() ) {
				strAddr_r(m_strBuf, sizeof(m_strBuf));
				return m_strBuf;
			}
			return "<not-set>";
		}

		virtual const char* c_str() const {
			return cs();
		}
};


#endif /* __SHELL_NETADDR_H_INCLUDED__ */
