/*
 *  Carbon framework
 *  Http protocol container
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 29.09.2016 16:39:37
 *      Initial revision.
 *      No chanking encoding support
 */

#ifndef __CARBON_HTTP_CONTAINER_H_INCLUDED__
#define __CARBON_HTTP_CONTAINER_H_INCLUDED__

#include "shell/socket.h"

#include "carbon/net_container.h"
#include "carbon/cstring.h"

#define HTTP_EOL					"\r\n"
#define HTTP_EOL_LEN				2
#define HTTP_CONTENT_LENGTH			"Content-Length"
#define HTTP_CONTENT_TYPE			"Content-Type"
#define HTTP_ACCEPT_ENCODING		"Content-Encoding"


class CHttpContainer : public CNetContainer
{
	protected:
		CString			m_strStart;			/* Start line without \r\n */
		CString			m_strHeader;		/* Headers separated by \r\n */

		uint8_t*		m_pBody;			/* Body buffer */
		size_t			m_nCurSize;			/* Current body size, bytes */
		size_t			m_nSize;			/* Allocated body buffer size, bytes */

	public:
		CHttpContainer();
		CHttpContainer(const char* strStart);
		CHttpContainer(const char* strStart, const char* strHeader);

	protected:
		virtual ~CHttpContainer();

	public:
		void setStartLine(const char* strStart);
		void appendHeader(const char* strHeader);
		void appendBody(const void* pBuffer, size_t size);

		boolean_t getHeader(CString& strValue, const char* strHeader = NULL) const;
		void deleteHeader(const char* strHeader = NULL);

		const char* getStart() const { return m_strStart; }

		void* getBody() { return m_pBody; }
		size_t getBodySize() const { return  m_nCurSize; }

		virtual void clear();
		virtual CNetContainer* clone();

		virtual result_t send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr = NETADDR_NULL);
		virtual result_t receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr = NULL);

		virtual void dump(const char* strPref = "") const;
		virtual void getDump(char* strBuf, size_t length) const;

	protected:
		void makeBuffer(size_t nSize);
		int findHeader(const char* strHeader) const ;
		result_t serialise(char** ppBuffer, size_t* pSize);
		virtual int getContentLength() const;
};

#endif /* __CARBON_HTTP_CONTAINER_H_INCLUDED__ */
