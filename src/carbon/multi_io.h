/*
 *  Carbon framework
 *  Multi packets I/O exchange
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.04.2015 12:33:24
 *      Initial revision.
 */

#ifndef __CARBON_MULTI_IO_H_INCLUDED__
#define __CARBON_MULTI_IO_H_INCLUDED__

#include "shell/hr_time.h"
#include "shell/socket.h"
#include "shell/netaddr.h"

#include "carbon/utils.h"
#include "carbon/net_container.h"

#define MIO_PACKETS_MAX		4096

typedef struct
{
	CNetAddr		netAddr;			/* Destination address */
	int				options;			/* Packet I/O options */

	CNetContainer*	pSendContainer;		/* Send packet container */
	CNetContainer*	pRecvContainer;		/* Receive packet container */

	result_t		nresult;			/* Packet I/O result */

	/* -- Interval use only -- */
	int				stage;
	CSocketAsync 	socket;
	uint32_t		sendOffset;
} multi_io_t;


class CMultiIo
{
	protected:
		char		m_strTitle[64];
		int			m_hBreaker;

	public:
		enum MioOption {
			mioSend		= 1,
			mioReceive	= 2
		};

	protected:
		enum MioStage {
			stageConnect	= 1,
			stageSend		= 2,
			stageReceive	= 3,
			stageCompleted	= 4
		};

	public:
		CMultiIo(const char* strTitle, int hBreaker = 0) :
			m_hBreaker(hBreaker)
		{
			shell_assert(strTitle && *strTitle != '\0');
			copyString(m_strTitle, strTitle, sizeof(m_strTitle));
		}

		virtual ~CMultiIo() {}

	public:
		void setBreaker(int hBreaker) {
			m_hBreaker = hBreaker;
		}

		virtual result_t multiIo(multi_io_t* arMio, int count, hr_time_t hrTimeout);

	protected:
		void resetBreaker();
		virtual void itemCompleted(multi_io_t* pMio, struct pollfd* pPoll, result_t nresult);
		virtual result_t doSend(multi_io_t* pMio);
		virtual result_t doReceive(multi_io_t* pMio);
};

#endif /* __CARBON_MULTI_IO_H_INCLUDED__ */
