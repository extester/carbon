/*
 *	Carbon/Network MultiMedia Streaming Module
 *	SDP protocol parser module
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *	Revision history:
 *
 *	Revision 1.0, 24.10.2016 15:43:57
 *		Initial revision.
 */

#ifndef __NET_MEDIA_SDP_H_INCLUDED__
#define __NET_MEDIA_SDP_H_INCLUDED__

#include <vector>

#include "shell/netaddr.h"

#include "carbon/carbon.h"
#include "carbon/cstring.h"

#define SDP_HTTP_MEDIA_TYPE					"application/sdp"

#define SDP_PACKETIZATION_SINGLE			0		/* Default packetization mode */
#define SDP_PACKETIZATION_NON_INTERLEAVE	1		/* Non-interlived mode */
#define SDP_PACKETIZATION_INTERLEAVE		2		/* Interlived mode */


class CSdpAttribute
{
	private:
		std::vector<CString>	m_arAttribute;	/* SDP Media attributes */

	public:
		CSdpAttribute()
		{
			m_arAttribute.reserve(10);
		}

		~CSdpAttribute()
		{
		}

	public:
		boolean_t isEmpty() const { return m_arAttribute.empty(); }
		void clear() { m_arAttribute.clear(); }

		void insert(const CString& strValue) {
			m_arAttribute.push_back(strValue);
		}

		result_t get(const char* strPrefix, CString& strAttr/*out*/) const;

		void dump(const char* strMargin) const;
};

class CSdpMedia
{
	private:
		CString					m_strMedia;			/* Media description */
		CString					m_strInfo;			/* Media information (i=) */
		CString					m_strConnectInfo;	/* Connection info (c=) */
		CSdpAttribute			m_attribute;		/* Media level attributes (a=) */

	public:
		CSdpMedia(const CString& strMedia) :
			m_strMedia(strMedia)
		{
		}

		~CSdpMedia()
		{
		}

	public:
		void setInfo(const CString& strInfo) { m_strInfo = strInfo; }
		void setConnectionInfo(const CString& strInfo) { m_strConnectInfo = strInfo; }
		void insertAttr(const CString& strAttr) { m_attribute.insert(strAttr); }

		result_t getMedia(CString& strMedia, int& port, int& numPorts,
					  CString& strProto, std::vector<int>& arProfiles) const;
		result_t getAttr(const char* strPrefix, CString& strValue/*out*/) const {
			return m_attribute.get(strPrefix, strValue);
		}
		result_t getRtpmap(int profile, CString& strEncoding, int& nClockRate,
						   CString& strParams) const;

		const char* getConnectionInfo() const { return m_strConnectInfo; }

		void dump(const char* strMargin) const;
};

class CSdp
{
	protected:
		int						m_nVersion;			/* SDP version (v=) (mandatory) */
		CString					m_strOriginator;	/* Session Creator (o=) (mandatory) */
		CString					m_strName;			/* Session Name (s=) (mandatory) */
		CString					m_strInfo;			/* Session information */
		CString					m_strUri;			/* Session URI */
		CString					m_strConnectInfo;	/* Connection info (c=) */
		CSdpAttribute			m_attribute;		/* Session level attributes (a=) */

		std::vector<CSdpMedia*>	m_arMedia;			/* Media streams */

	public:
		CSdp();
		virtual ~CSdp();

	public:
		size_t getMediaCount() const { return m_arMedia.size(); }

		void clear();
		result_t load(const char* strData);

		result_t getAttr(const char* strPrefix, CString& strValue) const {
			return m_attribute.get(strPrefix, strValue);
		}

		result_t getMedia(int index, CString& strMedia, int& port, int& numPorts,
						  CString& strProto, std::vector<int>& arProfiles) const;
		result_t getMediaConnectionInfo(int index, CString& strNetType,
						  CString& strAddrType, CNetHost& baseAddr, int& ttl, int& numAddr) const;
		result_t getMediaAttr(int index, const char* strPrefix, CString& strValue) const;
		result_t getMediaRtpmap(int index, int profile, CString& strEncoding,
								int& nClockRate, CString& strParams) const;

		void dump(const char* strPref = "") const;

	private:
		result_t parseLine(const char* strLine, char* pName, CString& strValue);
		result_t parseConnectionInfo(const CString& strConnectionInfo, CString& strNetType,
									 CString& strAddrType, CNetHost& baseAddr,
									 int& ttl, int& numAddr) const;
};

#endif /* __NET_MEDIA_SDP_H_INCLUDED__ */
