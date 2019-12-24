/*
 *  Carbon/Contact module
 *  Generic network interface configuration
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.08.2017 17:57:15
 *      Initial revision.
 */

#ifndef __IFCONFIG_H_INCLUDED__
#define __IFCONFIG_H_INCLUDED__

#include "shell/shell.h"
#include "shell/netaddr.h"

#include "carbon/cstring.h"

class CIfConfig
{
	protected:
		CString				m_strIface;		/* Interface name */

	public:
		explicit CIfConfig(const char* strIface) :
			m_strIface(strIface)
		{
		}

		virtual ~CIfConfig()
		{
		}

	public:
		void setIface(const char* strIface)  { m_strIface = strIface; }
		boolean_t isPresnt() const;

		result_t isUp(boolean_t& bUp) const;
		result_t up(boolean_t bUp = TRUE);

		result_t config(const CNetHost& ipAddr, const CNetHost& ipMask);
		result_t config(const char* strIpAddr, const char* strIpMask);
		//result_t config(const iface_config_t* pConfig);

		result_t getIp(CNetHost& ipAddr) const;
		result_t getMask(CNetHost& ipMask) const;

		result_t setMacAddr(const uint8_t* pMacData);
		result_t setMacAddr(const char* strMac);
		result_t getMacAddr(uint8_t* pMacAddr) const;
		result_t getMacAddr(CString& strMac) const;

	protected:
		result_t openSocket(int& hSocket) const;
		void closeSocket(int hSocket) const;
		result_t ioctl(int hSocket, unsigned long type, struct ifreq* ifr);
		result_t ioctl_const(int hSocket, unsigned long type, struct ifreq* ifr) const;
};

#endif /* __IFCONFIG_H_INCLUDED__ */
