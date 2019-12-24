/*
 *  Carbon framework
 *  Simple text (string-oriented) container
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 19.04.2015 16:35:06
 *      Initial revision.
 */

#ifndef __CARBON_TEXT_CONTAINER_H_INCLUDED__
#define __CARBON_TEXT_CONTAINER_H_INCLUDED__

#include "shell/socket.h"

#include "carbon/net_container.h"

#define TEXT_CONTAINER_INSIZE				PAGE_SIZE
#define TEXT_CONTAINER_EOL					"\r\n"

class CTextContainer : public CNetContainer
{
	protected:
		char*			m_pBuffer;
		char			m_inBuffer[TEXT_CONTAINER_INSIZE];
		size_t			m_nSize;
		size_t			m_nCurSize;
		size_t			m_nMaxSize;

		const char*		m_strEol;

	public:
		CTextContainer(size_t nMaxSize, const char* strEol = TEXT_CONTAINER_EOL);
	protected:
		virtual ~CTextContainer();

	public:
		operator const char*() const {
			return m_pBuffer;
		}

		virtual void clear();

		virtual result_t putData(const void* pData, size_t nSize);
		virtual result_t putData(const char* strData);

		virtual result_t send(CSocketAsync& socket, uint32_t* pOffset);
		virtual result_t receive(CSocketAsync& socket);

		virtual void dump(const char* strPref = "") const;

	protected:
		boolean_t isInline() const { return m_pBuffer == m_inBuffer; }
		result_t expandBuffer(size_t nDelta);
};

#endif /* __CARBON_TEXT_CONTAINER_H_INCLUDED__ */
