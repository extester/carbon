/*
 *  Carbon/Contact library
 *  SIM900R GSM modem 
 *
 *  Copyright (c) 2013-2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 05.03.2013, 18:11:39
 *      Initial revision.
 *
 *  Revision 1.1, 08.05.2018 15:41:19
 *  	Ported to carbon/contact.
 */

#ifndef __CONTACT_MODEM_SIM900R_H_INCLUDED__
#define __CONTACT_MODEM_SIM900R_H_INCLUDED__

#include "contact/modem_base.h"


#define MUX_N1				127		/* Maximum frame size */
#define MUX_T1				10
#define MUX_N2				3
#define MUX_T2				30
#define MUX_T3				10


/*
 * Received Unsolicited codes
 */
enum {
	UNSOLICITED_RDY				= 0x0001,		/* "RDY" */
	UNSOLICITED_RING			= 0x0002,		/* "RING" */
	UNSOLICITED_CALL_READY		= 0x0004		/* "Call Ready" */
};

/*
 * SIM Card state
 */
typedef enum {
	CARD_STATE_UNKNOWN 			= 0,
	CARD_STATE_ABSENT			= 1,
	CARD_STATE_PRESENT			= 2,
} card_state_t;


#define STAT_UNKNOWN			(-1)

class CModemSim900r : public CModemBase
{
	protected:
		uint16_t 			m_nUnsolicited;		/* Processed unsolicited events bitmap */
		card_state_t		m_nCardState;		/* Sim card state, CARD_STATE_xxx */
		int					m_nCFun;			/* Modem functionality */
		int					m_nCGReg;			/* GPRS Registration status */

	public:
		CModemSim900r(const char* strName);
		virtual ~CModemSim900r();

	public:
		card_state_t getCardState() const { return m_nCardState; }

		void clearUnsolicited(uint16_t nCode = 0xffff)  {
			m_nUnsolicited &= ~nCode;
		}

		virtual boolean_t isReady() const {
			return m_nCardState == CARD_STATE_PRESENT &&
				(m_nUnsolicited&UNSOLICITED_CALL_READY) != 0;
		}

		virtual result_t send(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout);
		virtual result_t recv(void* pBuffer, size_t* pSize, hr_time_t hrTimeout);

		virtual result_t sendCmd(const char* strCmd, hr_time_t hrTimeout);
		virtual result_t recvResp(void* pBuffer, size_t size, hr_time_t hrTimeout);

		virtual result_t exec0(const char* strCommand, void* pResponse, size_t sizeResp,
								hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout);
		virtual result_t execCmd0(const char* strCommand, void* pResponse, size_t sizeResp,
								hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout);
		virtual result_t execCmd1(const char* strCommand, void* pResponse, size_t sizeResp,
								hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout);

		/* High level functions */
		virtual result_t waitNetworkRegister(hr_time_t hrTimeout, boolean_t* pbRoaming = 0);
		virtual result_t readType(char* strType, size_t nLength);
		virtual result_t readRevision(char* strRevision, size_t nLength);
		virtual result_t readImei(char* strImei, size_t nLength);
		virtual	result_t readOperator(char* strOper, size_t nLength);
		virtual	result_t readSimId(char* strSimId, size_t nLength);
		virtual result_t readRssi(int* pnRssi, int* pnBer);

		virtual result_t writeBaudRate(int nBaudRate);
		virtual result_t saveUserDefinedProfile();

		/* GPRS Service */
		virtual result_t waitGprsRegister(hr_time_t hrTimeout, boolean_t* pbRoaming = 0);
		virtual result_t gprsAttach(hr_time_t hrTimeout);
		virtual result_t gprsDetach();

		virtual result_t gprsOpenIpSession(const char* strApn, const char* strUser,
							const char* strPass, hr_time_t hrTimeout, char* strIpLocal = 0);
		virtual result_t gprsCloseIpSession();

		/* HTTP Service */
		virtual result_t httpOpenSession(hr_time_t hrTimeout);
		virtual result_t httpCloseSession();
		virtual result_t httpGetAction(const char* strUrl, hr_time_t hrTimeout,
									   size_t* pLength, int* pHttpCode = NULL);

		virtual result_t httpRead(void* pBuffer, size_t* pSize, size_t offset, hr_time_t hrTimeout);

		/* RTC Support */
		virtual result_t getClock(hr_time_t* pTime, hr_time_t* pTimestamp);
		virtual result_t setClock(time_t nTime);

		/* GSM Mux support */
		virtual result_t enableMux(int nBaudRate);

	protected:
		virtual result_t doSend(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout);
		virtual result_t doRecv(void* pBuffer, size_t* pSize, hr_time_t hrTimeout);
		virtual result_t doRecvRaw(void* pBuffer, size_t* pSize, hr_time_t hrTimeout);

		virtual result_t initModem(int nBaudRate, hr_time_t hrTimeout);

		/* Unsolicited message handlers */
		virtual void onRing() {}
		virtual void onCallReady() {}
		virtual void onCpinReady() {}
		virtual void onNotInserted() {}

		virtual boolean_t doUnsolicited(const char* strResponse, size_t size);
		void parseResponse(char* pResponse, size_t size) const;
		boolean_t isFinishLine(const char* strResponse) const;

		void resetState() {
			m_nUnsolicited = 0;
			m_nCardState = CARD_STATE_UNKNOWN;
			m_nCFun = STAT_UNKNOWN;
			m_nCGReg = STAT_UNKNOWN;
		}
};

#endif /* __CONTACT_MODEM_SIM900R_H_INCLUDED__ */
