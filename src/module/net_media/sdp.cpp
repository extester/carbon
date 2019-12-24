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
 *	Revision 1.0, 24.10.2016 15:44:38
 *		Initial revision.
 */

#include "carbon/utils.h"

#include "net_media/sdp.h"

#define SDP_VERSION					0
#define SDP_VERSION_UNDEF			(-1)

/*******************************************************************************
 * CSdpAttribute class
 */

/*
 * Get a specified attribute value
 *
 * 		strPrefix		attribute prefix string ("rtpmap:")
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENOENT			attribute not found
 */
result_t CSdpAttribute::get(const char* strPrefix, CString& strAttr) const
{
	size_t		length, count, i;
	result_t	nresult;

	if ( strPrefix == 0 || strPrefix[0] == '\0' )  {
		return ENOENT;
	}

	nresult = ENOENT;
	length = _tstrlen(strPrefix);
	count = m_arAttribute.size();

	for(i=0; i<count; i++)  {
		const char*		s = m_arAttribute[i];
		size_t			l = _tstrlen(s);

		if ( l >= length && _tmemcmp(strPrefix, s, length) == 0 )  {
			strAttr = CString(s+length, l-length);
			nresult = ESUCCESS;
			break;
		}
	}

	return nresult;
}

void CSdpAttribute::dump(const char* strMargin) const
{
	size_t	count = m_arAttribute.size(), i;

	for(i=0; i<count; i++)  {
		log_dump("%s  a=%s\n", strMargin, m_arAttribute[i].cs());
	}
}


/*******************************************************************************
 * CSdpMedia class
 */

/*
 * Parse media description
 *
 * Format:
 *		m=<media> <port[/num_ports]> <proto> <profile_list>
 */
result_t CSdpMedia::getMedia(CString& strMedia, int& port, int& numPorts,
				  CString& strProto, std::vector<int>& arProfiles) const
{
	str_vector_t	arStr;
	size_t			size, i;
	const char*		s;
	result_t		nresult;

	size = strSplit(m_strMedia, ' ', &arStr);
	if ( size < 4 )  {
		return EINVAL;
	}

	/* Media */
	strMedia = arStr[0];

	/* Port/Count */
	s = _tstrchr(arStr[1], '/');
	if ( s != 0 )  {
		int 	n, xport, xports;

		n = _tsscanf(arStr[1], "%d/%d", &xport, &xports);
		if ( n == 2 )  {
			port = xport;
			numPorts = xports;
		}
		else {
			return EINVAL;
		}
	}
	else {
		if ( arStr[1].getNumber(port) != ESUCCESS )  {
			return EINVAL;
		}
		numPorts = 1;
	}

	/* Proto */
	strProto = arStr[2];

	/* Profiles */
	arProfiles.clear();

	nresult = ESUCCESS;
	for(i=3; i<size; i++)  {
		int 	profile;

		nresult = arStr[i].getNumber(profile);
		if ( nresult != ESUCCESS )  {
			break;
		}

		arProfiles.push_back(profile);
	}

	return nresult;
}

/*
 * Parse "a=rtpmap:<profile> <encoding>/clockrate[/params]>
 *
 */
result_t CSdpMedia::getRtpmap(int profile, CString& strEncoding, int& nClockRate,
				   CString& strParams) const
{
	char 			strTemp[64];
	CString			strValue;
	str_vector_t	arStr;
	size_t			size;
	int				n;
	result_t		nr;

	snprintf(strTemp, sizeof(strTemp), "rtpmap:%d ", profile);
	nr = getAttr(strTemp, strValue);
	if ( nr != ESUCCESS )  {
		return nr;
	}

	size = strSplit(strValue, '/', &arStr, " ");
	if ( size < 2 )  {
		return EINVAL;
	}

	/* Encoding name */
	strEncoding = arStr[0];

	/* Clock rate */
	if ( arStr[1].getNumber(n) != ESUCCESS )  {
		return EINVAL;
	}
	nClockRate = n;

	/* Other parameters */
	strParams = "";
	if ( size > 2 )  {
		strParams = arStr[2];
	}

	return ESUCCESS;
}

void CSdpMedia::dump(const char* strMargin) const
{
	log_dump("%sm=%s\n", strMargin, m_strMedia.cs());
	if ( !m_strInfo.isEmpty() )  {
		log_dump("%s  i=%s\n", strMargin, m_strInfo.cs());
	}
	if ( !m_strConnectInfo.isEmpty() )  {
		log_dump("%s  c=%s\n", strMargin, m_strConnectInfo.cs());
	}
	m_attribute.dump(strMargin);
}


/*******************************************************************************
 * CSdp class
 */

CSdp::CSdp() :
	m_nVersion(SDP_VERSION_UNDEF)
{
}

CSdp::~CSdp()
{
}

/*
 * Clear SPD object
 */
void CSdp::clear()
{
	size_t	i;

	m_nVersion = SDP_VERSION_UNDEF;
	m_strOriginator.clear();
	m_strName.clear();
	m_strInfo.clear();
	m_strUri.clear();
	m_strConnectInfo.clear();
	m_attribute.clear();

	for(i=0; i<m_arMedia.size(); i++)  {
		delete m_arMedia[i];
	}
	m_arMedia.clear();
}

/*
* Parse a single SDP "key=value" line
*
* 		strLine		text to parse
* 		pName		key [out]
* 		strValue	parse value [out]
*
* Return:
* 		ESUCCESS	parsed
* 		EINVAL		invalid format
* 		ENOENT		end of text
*/
result_t CSdp::parseLine(const char* strLine, char* pName, CString& strValue)
{
	const char	*s = strLine, *e;
	result_t	nresult = ENOENT;

	SKIP_CHARS(s, " \t");
	if ( *s != '\0' )  {
		*pName = *s;
		s++;
		if ( *s == '=' ) {
			s++;
			e = s;
			SKIP_NON_CHARS(e, "\r\n");
			strValue = CString(s, A(e) - A(s));
			nresult  = ESUCCESS;
		}
		else {
			log_debug(L_SDP, "[sdp] invalid format, no '=' found\n");
			nresult = EINVAL;
		}
	}

	return nresult;
}

/*
 * Load SDP data to the object
 *
 * 		strData		SDP data string
 *
 * Return: ESUCCESS, EINVAL
 */
result_t CSdp::load(const char* strData)
{
	const char*		s = strData;
	char 			key;
	CString			strValue;
	size_t			imedia = 0, n;
	int				ivalue;
	CSdpMedia*		pMedia;
	result_t		nresult = ESUCCESS, nr;

	enum {
		SDP_PARSE_SESSION,
		SDP_PARSE_MEDIA
	} stage = SDP_PARSE_SESSION;

	shell_assert(strData);
	shell_assert(m_attribute.isEmpty());
	shell_assert(m_nVersion == SDP_VERSION_UNDEF);

	while ( nresult == ESUCCESS && *s != '\0' )  {
		nresult = parseLine(s, &key, strValue);
		if ( nresult == ESUCCESS )  {
			if ( stage == SDP_PARSE_SESSION )  {
				switch ( key )  {
					case 'v':			/* Protocol version */
						nr = strValue.getNumber(ivalue);
						if ( nr == ESUCCESS )  {
							m_nVersion = ivalue;
						}
						else {
							log_debug(L_GEN, "[sdp] invalid version value '%s'\n",
									  strValue.cs());
							nresult = EINVAL;
						}
						break;

					case 'o':			/* Originator */
						m_strOriginator = strValue;
						break;

					case 's':			/* Session name */
						m_strName = strValue;
						break;

					case 'i':			/* Session information */
						m_strInfo = strValue;
						break;

					case 'u':			/* Session URI */
						m_strUri = strValue;
						break;

					case 'c':			/* Connection information */
						m_strConnectInfo = strValue;
						break;

					case 'a':			/* Session attribute */
						m_attribute.insert(strValue);
						break;

					case 'm':			/* Start the first media description parameters */
						stage = SDP_PARSE_MEDIA;
						imedia = 0;
						pMedia = new CSdpMedia(strValue);
						m_arMedia.push_back(pMedia);
						break;

					default:
						/* Ignore unknown/unsupported parameters */
						break;
				}
			} else {
				switch ( key )  {
					case 'i':			/* Media information */
						m_arMedia[imedia]->setInfo(strValue);
						break;

					case 'c':			/* Connection information */
						m_arMedia[imedia]->setConnectionInfo(strValue);
						break;

					case 'a':			/* Media attribute */
						m_arMedia[imedia]->insertAttr(strValue);
						break;

					case 'm':			/* Create new media description */
						imedia++;
						pMedia = new CSdpMedia(strValue);
						m_arMedia.push_back(pMedia);
						break;

					default:
						/* Ignore unknown/unsupported parameters */
						break;
				}
			}

			n = _tstrcspn(s, "\n\r");
			s += n;
			SKIP_CHARS(s, "\n\r");
		}
	}

	if ( nresult == ENOENT )  {
		nresult = ESUCCESS;
	}

	if ( nresult == ESUCCESS )  {
		/*
		 * Validate loaded description
		 */
		nresult = EINVAL;

		/* Mandatory: v=0, o=, s=, t= */
		if ( m_nVersion != SDP_VERSION ) {
			log_debug(L_SDP, "[sdp] invalid sdp version\n");
		} else
		if ( m_strOriginator.isEmpty() )  {
			log_debug(L_SDP, "[sdp] no Originator specified\n");
		} else
		if ( m_strName.isEmpty() )  {
			log_debug(L_SDP, "[sdp] no session name specified\n");
		}
		else {
			/* At least one session must be defined */
			if ( m_arMedia.size() < 1 ) {
				log_debug(L_SDP, "[sdp] no media defined\n");
			}
			else {
				nresult = ESUCCESS;
			}
		}
	}

	return nresult;
}

/*
 * Obtain media information
 *
 * 		index			media index, 0...n
 * 		strMedia		media type [out] ("video"/"audio")
 * 		port			transport port [out]
 * 		numPorts		transport port count [out]
 * 		strProto		transport protocol [out] ("RTP/AVP")
 * 		arProfiles		profiles [out]
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENOENT			media index is out of range
 * 		EINVAL			media description is invalid
 */
result_t CSdp::getMedia(int index, CString& strMedia, int& port, int& numPorts,
				  CString& strProto, std::vector<int>& arProfiles) const
{
	result_t	nresult;

	if ( index >= (int)m_arMedia.size() )  {
		return ENOENT;
	}

	nresult = m_arMedia[index]->getMedia(strMedia, port, numPorts, strProto, arProfiles);
	return nresult;
}

/*
 * Parse connection information string
 *
 * Format:
 * 		c=<nettype> <addrtype> <baseaddr[/ttl/[addresses]]>
 */
result_t CSdp::parseConnectionInfo(const CString& strConnectionInfo, CString& strNetType,
						CString& strAddrType, CNetHost& baseAddr, int& ttl, int& numAddr) const
{
	str_vector_t	arStr, arStrAddr;
	size_t			size, sizeAddr;
	CNetHost		netHost;
	int 			n;
	result_t		nr;

	size = strSplit(strConnectionInfo, ' ', &arStr);
	if ( size != 3 )  {
		return EINVAL;
	}

	/* Nettype */
	strNetType = arStr[0];

	/* Addrtype */
	strAddrType = arStr[1];

	/* Address */
	sizeAddr = strSplit(arStr[2], '/', &arStrAddr);

	ttl = 0;
	numAddr = 1;

	/* Address - base address */
	netHost = (const char*)arStrAddr[0];
	if ( !netHost.isValid() ) {
		return EINVAL;
	}
	baseAddr = netHost;

	/* Address - TTL */
	if ( sizeAddr > 1 )  {
		nr = arStrAddr[1].getNumber(n);
		if ( nr != ESUCCESS || (n < 0 || n > 255) )  {
			return EINVAL;
		}
		ttl = n;

		/* Address - count */
		if ( sizeAddr > 2 )  {
			nr = arStrAddr[2].getNumber(n);
			if ( nr != ESUCCESS || (n < 0 || n > 0xffff) ) {
				return EINVAL;
			}
			numAddr = n;
		}
	}

	return ESUCCESS;
}

/*
 * Get a media connection information
 *
 * 		index			media index, 0..n
 * 		strNetType		network type [out] ("IN")
 * 		strAddrType		connection address type [out] ("IP4"/"IP6")
 * 		baseAddr		connection address [out], ("1.2.3.4")
 * 		ttl				connection TTL [out] (multicast optional)
 * 		numAddr			multicast addresses (multicast optional, default 1)
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENOENT			media index is out of range
 * 		ENOSYS			connection info is missing
 * 		EINVAL			connection info is invalid
 */
result_t CSdp::getMediaConnectionInfo(int index, CString& strNetType, CString& strAddrType,
									  CNetHost& baseAddr, int& ttl, int& numAddr) const
{
	CString		strConnectionInfo;
	result_t	nresult;

	if ( index >= (int)m_arMedia.size() )  {
		return ENOENT;	/* Invalid media index */
	}

	strConnectionInfo = m_arMedia[index]->getConnectionInfo();

	if ( strConnectionInfo.isEmpty() )  {
		strConnectionInfo = m_strConnectInfo;
	}

	if ( strConnectionInfo.isEmpty() )  {
		return ENOSYS;	/* No connection info found */
	}

	nresult = parseConnectionInfo(strConnectionInfo, strNetType, strAddrType,
								  baseAddr, ttl, numAddr);
	return nresult;
}

/*
 * Obtain a media attribute
 *
 * 		index			media index, 0..n
 * 		strPrefix		attribute value prefix
 * 		strValu			attribute value (string following prefix) [out]
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENOENT			media index is out of range
 */
result_t CSdp::getMediaAttr(int index, const char* strPrefix, CString& strValue) const
{
	result_t	nresult;

	if ( index >= (int)m_arMedia.size() )  {
		return ENOENT;
	}

	nresult = m_arMedia[index]->getAttr(strPrefix, strValue);
	return nresult;
}

/*
 * Obtain a media rtpmap attribute parameters
 *
 * 		index			media index, 0..n
 * 		profile			profile number to get parameters
 * 		strEncoding
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENOENT			media index is out of range
 * 		EINVAL			parameters are invalid
 */
result_t CSdp::getMediaRtpmap(int index, int profile, CString& strEncoding,
						int& nClockRate, CString& strParams) const
{
	result_t	nresult;

	if ( index >= (int)m_arMedia.size() )  {
		return ENOENT;
	}

	nresult = m_arMedia[index]->getRtpmap(profile, strEncoding, nClockRate, strParams);
	return nresult;

}


/*******************************************************************************
 * Debugging support
 */

void CSdp::dump(const char* strPref) const
{
	size_t		count = m_arMedia.size(), i;

	log_dump("*** SDP %s(%u media(s)):\n", strPref, count);
	log_dump("    v=%d\n", m_nVersion);
	log_dump("    o=%s\n", m_strOriginator.cs());
	log_dump("    s=%s\n", m_strName.cs());
	if ( !m_strInfo.isEmpty() ) {
		log_dump("    i=%s\n", m_strInfo.cs());
	}
	if ( !m_strUri.isEmpty() )  {
		log_dump("    u=%s\n", m_strUri.cs());
	}
	if ( !m_strConnectInfo.isEmpty() )  {
		log_dump("    c=%s\n", m_strConnectInfo.cs());
	}
	m_attribute.dump("    ");

	for(i=0; i<count; i++)  {
		m_arMedia[i]->dump("    ");
	}
}
