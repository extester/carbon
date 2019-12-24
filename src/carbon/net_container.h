/*
 *  Carbon framework
 *  Base network container
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 19.04.2015 15:11:59
 *      Initial revision.
 */

#ifndef __CARBON_NET_CONTAINER_H_INCLUDED__
#define __CARBON_NET_CONTAINER_H_INCLUDED__

#include "shell/config.h"
#include "shell/socket.h"
#include "shell/ref_object.h"

#if CARBON_DEBUG_TRACK_OBJECT
#include "shell/track_object.h"
#endif /* CARBON_DEBUG_TRACK_OBJECT */

#if CARBON_DEBUG_TRACK_OBJECT
#define __CNetContainer_PARENT			,public CTrackObject<CNetContainer>
#else /* CARBON_DEBUG_TRACK_OBJECT */
#define __CNetContainer_PARENT
#endif /* CARBON_DEBUG_TRACK_OBJECT */


class CNetContainer : public CRefObject __CNetContainer_PARENT
{
	protected:
		CNetContainer() : CRefObject()/*, CTrackObject<CNetContainer>()*/ {}
		virtual ~CNetContainer() {}

	public:
		virtual void clear() {}
		virtual CNetContainer* clone() = 0;

		virtual result_t send(CSocket& socket, hr_time_t hrTimeout, const CNetAddr& dstAddr = NETADDR_NULL) = 0;
		virtual result_t receive(CSocket& socket, hr_time_t hrTimeout, CNetAddr* pSrcAddr = NULL) = 0;

		/* Optional */
		virtual result_t putData(const void* pData, size_t nSize)  { shell_assert(FALSE); return ENOSYS; }
		virtual result_t send(CSocketAsync& socket, uint32_t* pOffset) { shell_assert(FALSE); return ENOSYS; }
		virtual result_t receive(CSocketAsync& socket) { shell_assert(FALSE); return ENOSYS; }

#if CARBON_DEBUG_DUMP
		virtual void getDump(char* strBuf, size_t length) const = 0;
		virtual void dump(const char* strPref = "") const = 0;
#else /* CARBON_DEBUG_DUMP */
		virtual void getDump(char* strBuf, size_t length) const {
			if ( length > 0 ) {
				*strBuf = '\0';
			}
		}
		virtual void dump(const char* strPref = "") const { UNUSED(strPref); };
#endif /* CARBON_DEBUG_DUMP */
};

#endif /* __CARBON_NET_CONTAINER_H_INCLUDED__ */
