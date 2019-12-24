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
 *  Revision 1.0, 05.03.2013, 18:13:52
 *      Initial revision.
 *
 *  Revision 1.1, 08.05.2018 15:42:08
 *  	Ported to carbon/contact.
 *
 *  Revision 1.2, 19.09.2018 14:27:39
 *  	Added readType(), readRevision(), readImei(),
 *  	readOperator(), readSimId, readRssi().
 */
/*
 * Command syntax:
 *
 * 		<CR>		0x0d (13), '\r'
 * 		<LF>		0x0a (10), '\n'
 *
 * 		1) Command syntax:
 * 			AT<CMD><CR>
 *
 * 		2) Response syntax:
 * 			<CR><LF>Response<CR><LF>
 *
 * Initialisation:
 *
 * 		1) A HEX string "00 49 49 49 49 FF FF FF FF" will be sent
 * 		   through serial port at the baud 115200 immediately after power on.
 * 		   Shell be ignored.
 *
 * 		2) After power on the modem is in auto-bauding mode, synchronisation
 * 		   is necessary.
 *
 * Auto-bauding synchronisation:
 *
 * 		1) Wait 3 to 5 seconds after power on
 *
 * 		2) Auto-bauding port parameters: 8 data bit, no parity, 1 stop bit
 *
 * 		3) Echo mode is on by default
 *
 * 		3) Send 'AT' until you receive 'OK' response
 *
 * 		4) Issue 'AT+IPR=<baud>' command
 *
 *
 * Power on real initialisation example (sim card is present):
 *
 * 		1) IIII<cr><lf> or
 * 		   IIII'\xff'<cr><lf> or
 * 		   '\x00'IIII<cr><lf> or
 * 		   '\x00'IIII'\xff'<cr><lf>
 *
 *		2) RDY<cr><lf>
 *
 *		3) +CFUN: 1<cr><lf>
 *
 *		4) +CPIN: Ready<cr><lf>
 *
 *		5) +CGREG: 0<cr><lf>
 *
 *		6) +CREG: 1<cr><lf>
 */

#include <ctype.h>

#include "shell/memory.h"
#include "shell/utils.h"

#include "carbon/logger.h"

#include "contact/modem_sim900r.h"

#define MODEM_UNSOLICITED_BUFFER			128

#define SEP_CR								'\r'
#define SEP_LN								'\n'
#define SEP_LENGTH							2
#define IS_SEP(__c)							((__c)==SEP_CR||(__c)==SEP_LN)


/*******************************************************************************
 * Class CModemSim900r
 */

CModemSim900r::CModemSim900r(const char* strName) :
	CModemBase(strName),
	m_nUnsolicited(0),
	m_nCardState(CARD_STATE_UNKNOWN),
	m_nCFun(STAT_UNKNOWN),
	m_nCGReg(STAT_UNKNOWN)
{
}

CModemSim900r::~CModemSim900r()
{
}

/*
 * Table contains the formats of the last line response
 */
struct {
	const char* 	strResponse;		/* String */
	size_t			length;				/* Length, 0 - compare full string */
} const arFinishLine[] = {
		{ "OK",				0  },
		{ "ERROR",			0  },
		{ "+CME ERROR ",	11 },
		{ "+CME ERROR:",	11 },
		{ "+CMS ERROR ",	11 },
		{ "+CMS ERROR:",	11 },
		{ "NO CARRIER",		0  },
		{ "NO DIALTONE",	0  },
		{ "NO ANSWER",		0  },
		{ "BUSY",			0  },
		{ "CONNECT ",		8, }
};

/*
 * Check if a response string is last command response and
 * no following lines are expected
 *
 * 		strResponse		response ASCIIZ string
 *
 * Return: TRUE: last line, FALSE: is not last line
 */
boolean_t CModemSim900r::isFinishLine(const char* strResponse) const
{
	size_t		i, count = ARRAY_SIZE(arFinishLine);
	boolean_t	bFinish = FALSE;

	for(i=0; i<count && !bFinish; i++)  {
		if ( arFinishLine[i].length == 0 )  {
			bFinish = _tstrcmp(strResponse, arFinishLine[i].strResponse) == 0;
		}
		else {
			bFinish = _tmemcmp(strResponse, arFinishLine[i].strResponse,
							 arFinishLine[i].length) == 0;
		}
	}

	return bFinish;
}

/*
 * Power on modem initialisation
 *
 *		nBaudRate		baud rate, i.e. 115200	
 * 		hrTimeout		maximum init time
 *
 * Return:
 * 		ESUCCESS		init success
 * 		ETIMEDOUT		init is not completed in a given time
 * 		EINTR			interupted by user
 * 		...
 *
 * Note: assume a modem is powered on and opened.
 */
result_t CModemSim900r::initModem(int nBaudRate, hr_time_t hrTimeout)
{
	char			buffer[128];
	hr_time_t		hrStart;
	boolean_t		bAutobaud = FALSE;
	size_t			size;
	result_t		nresult = ETIMEDOUT, nr;

	log_debug(L_MODEM_FL, "[modem(%s)] hw modem initialising...\n", getName());
	
	resetState();

	/*
	 * Sleep 3..5 seconds for hardware boot up
	 */
	if ( hr_sleep(HR_5SEC) != ESUCCESS )  {
		return EINTR;
	}

	hrStart = hr_time_now();
	while ( hr_timeout(hrStart, hrTimeout) > HR_0 )
	{
		if ( !bAutobaud ) {
			nr = exec0("AT\r", buffer, sizeof(buffer), HR_500MSEC, HR_1SEC /*HR_500MSEC*/);
			if ( nr == ESUCCESS ) {
				if ( _tstrcmp(buffer, "\r\nOK\r\n") == 0 ) {
					/* Autobaud success, set fixed baud rate */
					_tsnprintf(buffer, sizeof(buffer), "AT+IPR=%d\r", nBaudRate);
					nr = exec0(buffer, buffer, sizeof(buffer), HR_2SEC, HR_2SEC);
					if ( nr == ESUCCESS )  {
						if ( _tstrcmp(buffer, "\r\nOK\r\n") == 0 )  {
							log_debug(L_MODEM_FL, "[modem(%s)] autobauding completed\n", getName());
							bAutobaud = TRUE;
							nresult = ESUCCESS;	/* !!! */
						}
					}
				}
			}
		}
		else {
			/* Process unsolicited messages */
			size = sizeof(buffer);
			nr = recv(buffer, &size, HR_500MSEC);
			if ( nr == ESUCCESS )  {
				doUnsolicited(buffer, size);
			}
		}

		if ( nr == EINTR )  {
			/*
			 * Interrupted by user
			 */
			log_debug(L_MODEM_FL, "[modem(%s)] initialisation sequency interrupted\n", getName());
			nresult = nr;
			break;
		}

		if ( !bAutobaud ) {
			continue;
		}

		if ( m_nCardState == CARD_STATE_ABSENT ) {
			/*
			 * Init completed successfully,
			 * sim card is not inserted
			 */
			nresult = ESUCCESS;
			break;
		}

		if ( m_nCardState == CARD_STATE_PRESENT &&
				(m_nUnsolicited&UNSOLICITED_CALL_READY) != 0 ) {
			/*
			 * Init completed successfully,
			 * sim card is inserted, call ready
			 */
			nresult = ESUCCESS;
			break;
		}
	}

	if ( nresult == ESUCCESS ) {
		log_debug(L_MODEM_FL, "[modem(%s)] hw init success, sim card is %s\n",
				  	getName(), m_nCardState == CARD_STATE_PRESENT ? "inserted" : "not inserted");
	}
	else {
		log_error(L_MODEM, "[modem(%s)] hw modem init failure, result: %d\n", getName(), nresult);
	}

	return nresult;
}

/*
 * Process unsolicited response
 *
 * 		strMessage		unsolicited ASCIIZ message
 *
 * Return:
 * 		TRUE		this is known unsolicited message
 *		FALSE		unknown message
 */
boolean_t CModemSim900r::doUnsolicited(const char* strMessage, size_t size) 
{
	boolean_t bProcessed = FALSE;

	if ( size > 4 ) {
		if ( _tstrcmp(strMessage, "RDY\r\n") == 0 ) {
			m_nUnsolicited |= UNSOLICITED_RDY;
			bProcessed = TRUE;
		} else if ( size > 5 ) {
			if ( _tstrcmp(strMessage, "RING\r\n") == 0 ) {
				m_nUnsolicited |= UNSOLICITED_RING;
				onRing();
				bProcessed = TRUE;
			} else if ( size > 5 ) {
				/* Check for [00]* IIII [ff]* */
				boolean_t	isIIII = FALSE;
				const char*	s = strMessage;

				while ( *s == '\0' ) s++;
				if ( *s == '\x49' && *(s+1) == '\x49' && *(s+2) == '\x49' &&
					 *(s+3) == '\x49' && (*(s+4) == '\xff' || *(s+4) == '\r') ) {
					isIIII = TRUE;
				}

				if ( isIIII ) {
					bProcessed = TRUE;
				} else
				if ( _tmemcmp(strMessage, "\r\n+CFUN: ", 9) == 0 ) {
					m_nCFun = atoi(strMessage + 9);
					bProcessed = TRUE;
				} else if ( size > 12 )  {
					if ( _tmemcmp(strMessage, "\r\n+CGREG: ", 10) == 0 ) {
						m_nCGReg = atoi(strMessage + 10);
						bProcessed = TRUE;
					} else if ( size > 13 ) {
						if ( _tstrcmp(strMessage, "\r\nCall Ready\r\n") == 0 ) {
							m_nUnsolicited |= UNSOLICITED_CALL_READY;
							onCallReady();
							bProcessed = TRUE;
						} else if ( size > 15 ) {
							if ( _tstrcmp(strMessage, "\r\n+CPIN: READY\r\n") == 0 ) {
								m_nCardState = CARD_STATE_PRESENT;
								onCpinReady();
								bProcessed = TRUE;
							} else if ( size > 22 ) {
								if ( _tstrcmp(strMessage, "\r\n+CPIN: NOT INSERTED\r\n") == 0 ) {
									m_nCardState = CARD_STATE_ABSENT;
									clearUnsolicited(UNSOLICITED_CALL_READY);
									onNotInserted();
									bProcessed = TRUE;
								}
							}
						}
					}
				}
			}
		}
	}

	if ( logger_is_enabled(LT_DEBUG|L_MODEM_IO) && bProcessed ) {
		char	tmp[128];
		_tsnprintf(tmp, sizeof(tmp), "[modem(%s)] URC", getName());
		dumpModemLine(strMessage, size, tmp);
	}

	return bProcessed;
}

/*
 * Send a single raw message
 *
 * 		pBuffer			message buffer
 * 		pSize			IN: message length, bytes
 * 						OUT: send bytes
 *		hrTimeout	 	maximum send time
 *
 * Return: ESUCCESS, ETIMEDOUT, EINVAL, EIO, ...
 */
result_t CModemSim900r::doSend(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout)
{
	result_t	nresult;

	shell_assert(pBuffer);
	shell_assert(pSize);

	if ( logger_is_enabled(LT_DEBUG|L_MODEM_IO) ) {
		char	tmp[128];
		_tsnprintf(tmp, sizeof(tmp), "[modem(%s)] send", getName());
		dumpModemLine(pBuffer, *pSize, tmp);
	}

	if ( (*pSize) < 1 )  {
		log_error(L_MODEM, "[modem(%s)] empty send buffer!\n", getName());
		return EINVAL;
	}

	nresult = deviceSend(pBuffer, pSize, hrTimeout);
	return nresult;
}

/*
 * Receive a single raw message (possible limited by buffer size)
 *
 * 		pBuffer			output buffer
 * 		pSize			IN: output buffer max size (including terminating '\0')
 * 						OUT: received bytes (excluding terminating '\0')
 * 		hrTimeout		maximum receive time
 *
 * Return: ESUCCESS, ETIMEDOUT, EINVAL, EIO, ...
 *
 * Processed message formats
 *
 * 		1) Response:		<cr><lf>message<cr><lf>
 * 		2) Unsolicited:		[<cr><lf>]message<cr><lf>
*		3) Command echo:	ATsss<cr> or A/<cr>
 */
result_t CModemSim900r::doRecv(void* pBuffer, size_t* pSize, hr_time_t hrTimeout)
{
	unsigned char*	pBuf = (unsigned char*)pBuffer, byte;
	int				maxlength = (int)(*pSize)-1, length = 0;
	hr_time_t		hrStart, hrIter;
	result_t		nresult = ESUCCESS;

	shell_assert(maxlength >= 0);
	shell_assert(pBuffer);
	hrStart = hr_time_now();

	while( length < maxlength ) {
		hrIter=hr_timeout(hrStart, hrTimeout);
		
		if ( hrIter == HR_0 && length > 0 )  {
			hrIter = HR_200MSEC;	/* Add 200 ms to complete transfer */
		}
		
		if ( hrIter == HR_0 )  {
			break;
		}
			
		nresult = deviceRecv(&byte, hrIter);
		if ( nresult != ESUCCESS ) {
			break;
		}

		pBuf[length] = byte;
		length++;

		if ( byte == SEP_CR && length > 2 && toupper(pBuf[0]) == 'A' &&
				(toupper(pBuf[1]) == 'T' || pBuf[1] == '/') )  {
			/* Command echo */		
			break;
		}					
		
		if ( byte == SEP_LN && length > 2 )  {
			/* Response or unsolicited */
			break;
		}
	}

	if ( (*pSize) > 0 )  {
		pBuf[length] = '\0';
	}
	else {
		nresult = EINVAL;
	}
	if ( nresult == ESUCCESS && length == 0 ) {
		nresult = ETIMEDOUT;
	} else if ( nresult == ETIMEDOUT && length > 0 )  {
		nresult = ESUCCESS;
	}

	*pSize = (size_t)length;

	if ( logger_is_enabled(LT_DEBUG|L_MODEM_IO) ) {
		char	tmp[128];
		_tsnprintf(tmp, sizeof(tmp), "[modem(%s)] recv", getName());
		dumpModemLine(pBuffer, length, tmp);
	}

	return nresult;
}

result_t CModemSim900r::doRecvRaw(void* pBuffer, size_t* pSize, hr_time_t hrTimeout)
{
	unsigned char*	pBuf = (unsigned char*)pBuffer;
	int				maxlength = (int)(*pSize), length = 0;
	hr_time_t		hrStart, hrIter;
	result_t		nresult = ESUCCESS;

	shell_assert(pBuffer);
	hrStart = hr_time_now();

	while( length < maxlength && (hrIter=hr_timeout(hrStart, hrTimeout)) > HR_0 )
	{
		nresult = deviceRecv(&pBuf[length], hrIter);
		if ( nresult != ESUCCESS ) {
			break;
		}

		length++;
	}

	*pSize = (size_t)length;
	if ( length < maxlength && nresult != ESUCCESS )  {
		nresult = ETIMEDOUT;
	}
	
	log_debug(L_MODEM_IO, "[modem(%s)] Recv: %d byte(s) of binary data\n", getName(), length);

	return nresult;
}

/*
 * Send a single message with echo processing
 *
 * 		pBuffer			message buffer
 * 		pSize			IN: message length, bytes
 * 						OUT: send bytes
 *		hrTimeout	 	maximum send time
 *
 * Return: ESUCCESS, ETIMEDOUT, EIO, ...
 */
result_t CModemSim900r::send(const void* pBuffer, size_t* pSize, hr_time_t hrTimeout)
{
	char*			strTmpBuffer;
	const size_t 	length = (*pSize);
	size_t			sizeBuf, sizeEcho;
	hr_time_t		hrStart, hrTimeoutEcho;
	result_t		nresult;

	hrStart = hr_time_now();
	nresult = doSend(pBuffer, pSize, hrTimeout);
	if ( nresult == ESUCCESS ) {
		/*
		 * Echo is always ON
		 * Make buffer long enough to receive unsolicited message
		 */
		sizeBuf = MAX(length, MODEM_UNSOLICITED_BUFFER);
		strTmpBuffer = (char*) alloca(sizeBuf+1);

		while ( nresult == ESUCCESS ) {
			hrTimeoutEcho = hr_timeout(hrStart, hrTimeout);
			sizeEcho = sizeBuf;
			nresult = doRecv(strTmpBuffer, &sizeEcho, hrTimeoutEcho);
			if ( nresult == ESUCCESS ) {
				if ( sizeEcho == length && _tmemcmp(pBuffer, strTmpBuffer, length) == 0 ) {
					/* Received echo */
					break;
				}

				/* Received unsolicited message */
				doUnsolicited(strTmpBuffer, sizeEcho);
			}
			else {
				log_debug(L_MODEM_FL, "[modem(%s)] failed to receive command ECHO, result: %d\n",
						  				getName(), nresult);
			}
		}
	}
	else {
		/*
		 * Send failed
		 */
		log_error(L_MODEM, "[modem(%s)] failed to send command, result: %d\n", getName(), nresult);
	}

	return nresult;
}

/*
 * Receive a single raw message (possible limited by buffer size)
 *
 * 		pBuffer			output buffer
 * 		pSize			IN: output buffer max size (including terminating '\0')
 * 						OUT: received bytes (excluding terminating '\0')
 * 		hrTimeout		maximum receive time
 *
 * Return: ESUCCESS, ETIMEDOUT, EINVAL, EIO, ...
 *
 * Processed message formats
 *
 * 		1) Response:		<cr><lf>message<cr><lf>
 * 		2) Unsolicited:		message<cr><lf>
 */
result_t CModemSim900r::recv(void* pBuffer, size_t* pSize, hr_time_t hrTimeout)
{
	result_t	nresult;

	nresult = doRecv(pBuffer, pSize, hrTimeout);
	return nresult;
}

result_t CModemSim900r::sendCmd(const char* strCmd, hr_time_t hrTimeout)
{
	char*			strBuf;
	const size_t	length = strlen(strCmd);
	size_t			size = length+1;
	result_t		nresult;

	strBuf = (char*)alloca(size+1);
	_tstrcpy(strBuf, strCmd);
	strBuf[length] = SEP_CR;
	strBuf[length+1] = '\0';

	nresult = send(strBuf, &size, hrTimeout);
	return nresult;
}

void CModemSim900r::parseResponse(char* pResponse, size_t size) const
{
	char	*p, *e;
	size_t	length;

	p = pResponse;
	while ( IS_SEP(*p) ) p++;

	e = p + size - (A(p)-A(pResponse));
	if ( e > p )  {
		e--;
		while ( e > p && IS_SEP(*e) )  e--;
		e++;
	}

	length = A(e) - A(p);
	_tmemmove(pResponse, p, length);
	pResponse[length] = '\0';
}

result_t CModemSim900r::recvResp(void* pBuffer, size_t size, hr_time_t hrTimeout)
{
	size_t		rsize = size;
	result_t	nresult;

	nresult = recv(pBuffer, &rsize, hrTimeout);
	if ( nresult == ESUCCESS ) {
		/*
	 	 * Received a raw response [<cr><lf>]ssss<cr><lf>
	 	 * Remove separators
	 	 */
		parseResponse((char*) pBuffer, rsize);
	}

	return nresult;
}

/*
 * Execute command with single response line (raw command and raw response)
 *
 * 		strCommand			command to execute ('AT\r')
 * 		pResponse			output response buffer ('<cr><lf>OK<cr><lf>)
 * 		respSize			response buffer size, bytes (including terminating '\0')
 * 		hrSendTimeout		sending maximum time (send command, receive echo, process unsolicited)
 * 		hrRecvResponse		reciving maximum time
 *
 * Return: ESUCCESS, EINVAL, ETIMEDOUT, EIO, ...
 *
 * Sample command:
 * 		send> AT
 * 			... unsolicited ...
 * 		resp> OK
 */
result_t CModemSim900r::exec0(const char* strCommand, void* pResponse, size_t sizeResp,
							 hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout)
{
	size_t		size;
	result_t	nresult;

	size = _tstrlen(strCommand);
	nresult = send(strCommand, &size, hrSendTimeout);
	if ( nresult == ESUCCESS )  {
		size = sizeResp;
		nresult = recv(pResponse, &size, hrRecvTimeout);
	}

	return nresult;
}

#define IS_RESP_OK(__s)		\
	(*(const char*)(__s) == 'O' && *((const char*)(__s)+1) == 'K' && *((const char*)(__s)+2) == '\0')

/*
 * Execute command with single response line
 *
 * 		strCommand			command to execute ('AT')
 * 		pResponse			output response buffer ('OK')
 * 		respSize			response buffer size, bytes (including terminating '\0')
 * 		hrSendTimeout		sending maximum time (send command, receive echo, process unsolicited)
 * 		hrRecvResponse		reciving maximum time
 *
 * Return:
 * 		ESUCCESS 		command executed and received 'OK' answer
 * 		ENXIO			command executed and received unexpected anser
 * 		ETIMEDOUT		timeout expired
 * 		EINVAL			invalid parameters
 * 		EIO				hardware I/O error
 *
 * Sample command:
 * 		send> AT
 * 			... unsolicited ...
 * 		resp> OK
 */
result_t CModemSim900r::execCmd0(const char* strCommand, void* pResponse, size_t sizeResp,
						  hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout)
{
	result_t	nresult;

	nresult = sendCmd(strCommand, hrSendTimeout);
	if ( nresult == ESUCCESS )  {
		nresult = recvResp(pResponse, sizeResp, hrRecvTimeout);
		if ( nresult == ESUCCESS && !IS_RESP_OK(pResponse) )  {
			nresult = ENXIO;
		}
	}

	return nresult;
}

/*
 * Execute command with double (single data) response lines
 *
 * 		strCommand			command to execute ('AT+CCI')
 * 		pResponse			output response buffer (data)
 * 		respSize			response buffer size, bytes (including terminating '\0')
 * 		hrSendTimeout		sending maximum time (send command, receive echo, process unsolicited)
 * 		hrRecvResponse		reciving maximum time
 *
 * Result:
 * 		ESUCCESS 		command executed and pResponse contains a parsed data string
 * 		ENXIO			1) received a single line answer; 2) second line is not 'OK'
 * 		ETIMEDOUT		timeout expired
 * 		EINVAL			invalid parameters
 * 		EIO				hardware I/O error
 *
 * Sample command:
 * 		send> AT+CCID
 * 			... unsolicited ...
 *		resp> data		(goes to response buffer)
 * 		resp> OK
 */
result_t CModemSim900r::execCmd1(const char* strCommand, void* pResponse, size_t sizeResp,
								 hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout)
{
	char*		pResponse2;
	hr_time_t	hrNow;
	result_t	nresult;

	nresult = sendCmd(strCommand, hrSendTimeout);
	if ( nresult == ESUCCESS )  {
		hrNow = hr_time_now();
		nresult = recvResp(pResponse, sizeResp, hrRecvTimeout);
		if ( nresult == ESUCCESS )  {
			if ( !isFinishLine((char*)pResponse) ) {
				pResponse2 = (char*)alloca(sizeResp);
				nresult = recvResp(pResponse2, sizeResp, hr_timeout(hrNow, hrRecvTimeout));
				if ( nresult == ESUCCESS && !IS_RESP_OK(pResponse2) )  {
					nresult = ENXIO;
				}
			}
			else {
				/*
				 * Expecting double line answer but received
				 * a single-line response
				 */
				nresult = ENXIO;
			}
		}
	}

	return nresult;
}

/*******************************************************************************
 * High level modem commands
 */

/*
 * Set default modem baut rate
 *
 * 		nBaudRate		baud rate to set (9600, 19200, 57600, ...)
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::writeBaudRate(int nBaudRate)
{
	char		cmd[64], buffer[64];
	result_t	nresult;

	_tsnprintf(cmd, sizeof(cmd), "AT+IPR=%u", nBaudRate);
	nresult = execCmd0(cmd, buffer, sizeof(buffer), HR_2SEC, HR_2SEC);
	return nresult;
}

/*
 * Write current settings to the default user profile
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::saveUserDefinedProfile()
{
	char		buffer[64];
	result_t	nresult;

	nresult = execCmd0("AT&W", buffer, sizeof(buffer), HR_2SEC, HR_5SEC);
	return nresult;
}

/*
 * Wait modem for network registration
 * 
 *		hrTimeout			maximum awaiting time
 *		pbRoaming			out: TRUE: roaming, FALSE: home network
 * 
 * Return:
 *		ESUCCESS			registration successful
 *		EAGAIN				not registered
 * 		EACCES				registration denied
 * 		ENOENT				sim card is not inserted
 * 		EINTR				interrupted
 */
result_t CModemSim900r::waitNetworkRegister(hr_time_t hrTimeout, boolean_t* pbRoaming)
{
	char		buffer[64];
	hr_time_t	hrInterval;
	result_t	nr, nresult = EAGAIN;
	int 		iter = 0;
	hr_time_t	hrNow = hr_time_now();

	log_debug(L_MODEM_FL, "[modem(%s)] network registration...\n", getName());

	while ( hr_timeout(hrNow, hrTimeout) > HR_0 )  {
		nr = execCmd1("AT+CREG?", buffer, sizeof(buffer), HR_2SEC, HR_2SEC);
		
		if ( getCardState() == CARD_STATE_ABSENT )  {
			nresult = ENOENT;
			break;
		}
		
		if ( nr == ESUCCESS )  {
			int		n, stat, count;
			
			count = _tsscanf(buffer, "+CREG: %d,%d", &n, &stat);
			if ( count == 2 )  {
				switch ( stat )  {
					case 0:		/* Searching */
					case 2:	
						break;
					
					case 1:		/* Registered */
						log_debug(L_MODEM_FL, "[modem(%s)] network registered\n", getName());
						nresult = ESUCCESS;
						if ( pbRoaming ) *pbRoaming = FALSE;
						break;
					
					case 5:		/* Registered, roaming */
						log_debug(L_MODEM_FL, "[modem(%s)] network registered (roaming)\n", getName());
						nresult = ESUCCESS;
						if ( pbRoaming ) *pbRoaming = TRUE;
						break;
					
					case 3:		/* Denied */
						log_debug(L_MODEM_FL, "[modem(%s)] network registration DENIED\n", getName());
						nresult = EACCES;
						break;
				}
			}
		}		
		
		if ( nresult != EAGAIN )  {
			break;
		}

		hrInterval = HR_1SEC;
		if ( iter >= 20 )  {
			hrInterval = HR_3SEC;
		} else if ( iter >= 10 )  {
			hrInterval = HR_2SEC;
		}

		if ( hr_sleep(hrInterval) == EINTR )  {
			log_debug(L_MODEM_FL, "[modem(%s)] network registration has been interrupted\n",
					  	getName());
			nresult = EINTR;
			break;
		}
	}
	
	return nresult;	
}

/*
 * Read modem type identification string
 *
 * 		strType			type string [out]
 * 		nLength			strType buffer maximum length, bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::readType(char* strType, size_t nLength)
{
	char		strTmp[128];
	result_t	nresult;

	nresult = execCmd1("AT+CGMM", strTmp, sizeof(strTmp), HR_2SEC, HR_2SEC);
	if ( nresult == ESUCCESS ) {
		copyString(strType, strTmp, nLength);
		log_debug(L_MODEM_FL, "[modem(%s)] got modem type: '%s'\n", getName(), strTmp);
	}

	return nresult;
}

/*
 * Read modem firmware revision string
 *
 * 		strRevision		revision string [out]
 * 		nLength			strRevision buffer maximum length, bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::readRevision(char* strRevision, size_t nLength)
{
	char		strTmp[128];
	result_t	nresult;

	nresult = execCmd1("AT+CGMR", strTmp, sizeof(strTmp), HR_2SEC, HR_2SEC);
	if ( nresult == ESUCCESS ) {
		/* Response format: Revision:<revision> */
		if ( _tstrlen(strTmp) > 9 )  {
			copyString(strRevision, &strTmp[9], nLength);
		}
		else {
			copyString(strRevision, strTmp, nLength);
		}

		log_debug(L_MODEM_FL, "[modem(%s)] got modem firmware revision: '%s'\n",
				  getName(), strRevision);
	}

	return nresult;
}

/*
 * Read modem IMEI string
 *
 * 		strImei			IMEI string [out]
 * 		nLength			strImei buffer maximum length, bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::readImei(char* strImei, size_t nLength)
{
	char		strTmp[128];
	result_t	nresult;

	nresult = execCmd1("AT+GSN", strTmp, sizeof(strTmp), HR_2SEC, HR_2SEC);
	if ( nresult == ESUCCESS ) {
		copyString(strImei, strTmp, nLength);
		log_debug(L_MODEM_FL, "[modem(%s)] got imei: '%s'\n", getName(), strTmp);
	}

	return nresult;
}

/*
 * Retrieve mobile operator Id
 *
 * 		strOper			operator string [out]
 * 		nLength			strOper buffer maximum length, bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::readOperator(char* strOper, size_t nLength)
{
	char		strBuf[128], *s;
	result_t	nresult;

	nresult = execCmd1("AT+COPS?", strBuf, sizeof(strBuf), HR_2SEC, HR_2SEC);
	if ( nresult == ESUCCESS )  {
		nresult = EINVAL;

		s = _tstrchr(strBuf, ',');
		if ( s )  {
			s = _tstrchr(s+1, ',');
			if ( s )  {
				SKIP_CHARS(s, ", \"");
				if ( *s != '\0' )  {
					size_t		l;

					l = _tstrcspn(s, "\",");
					if ( l > 0 )  {
						copySubstring(strOper, s, l, nLength);
					}
					else {
						copyString(strOper, s, nLength);
					}

					nresult = ESUCCESS;
					log_debug(L_MODEM_FL, "[modem(%s)] got operator: '%s'\n", getName(), strOper);
				}
			}
		}
	}

	if ( nresult != ESUCCESS )  {
		log_debug(L_MODEM, "[modem(%s)] obtaining network operator failed, result: %d\n",
				  getName(), nresult);
	}

	return nresult;
}

/*
 * Retrieve sim card Id
 *
 * 		strSimId		SIM identification string [out]
 * 		nLength			strSimId buffer maximum length, bytes
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::readSimId(char* strSimId, size_t nLength)
{
	char		strTmp[128];
	result_t	nresult;

	nresult = execCmd1("AT+CCID", strTmp, sizeof(strTmp), HR_2SEC, HR_2SEC);
	if ( nresult == ESUCCESS ) {
		if ( _tstrlen(strTmp) > 8 && _tmemcmp(strTmp, "+CCID: ", 7) == 0 ) {
			/* Reply format: "+CCID: <id>" */
			copyString(strSimId, strTmp+7, nLength);
		}
		else {
			/* Reply format: "<id>" */
			copyString(strSimId, strTmp, nLength);
		}

		log_debug(L_MODEM_FL, "[modem(%s)] got sim id: '%s'\n", getName(), strSimId);
	}
	else {
		log_debug(L_MODEM_FL, "[modem(%s)] obtaining sim id failed, result: %d\n",
				  getName(), nresult);
	}

	return nresult;
}

/*
 * Retrieve sim quality report
 *
 * 		pnRssi			signal strength indication [out], may be NULL
 * 		pnBer			bit error rate [out], may be NULL
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::readRssi(int* pnRssi, int* pnBer)
{
	char		strTmp[128];
	result_t	nresult;

	nresult = execCmd1("AT+CSQ", strTmp, sizeof(strTmp), HR_2SEC, HR_2SEC);
	if ( nresult == ESUCCESS ) {
		if ( _tmemcmp(strTmp, "+CSQ:", 5) == 0 )  {
			int		count, rssi, ber;

			count = _tsscanf(&strTmp[5], "%d,%d", &rssi, &ber);
			if ( count == 2 )  {
				if ( pnRssi )	{ *pnRssi = rssi; }
				if ( pnBer )	{ *pnBer = ber; }
			}
			else {
				log_error(L_MODEM, "[modem(%s)] can't parse RSSI number, '%s'\n",
						  getName(), strTmp);
				nresult = EINVAL;
			}
		}
		else {
			log_error(L_MODEM, "[modem(%s)] can't parse RSSI, '%s'\n",
					  getName(), strTmp);
			nresult = EINVAL;
		}
	}
	else {
		log_error(L_MODEM, "[modem(%s)] can't read RSSI, result %d\n", getName(), nresult);
	}

	return nresult;
}

/*
 * Wait for GPRS registration
 * 
 *		hrTimeout			maximum awaiting time
 *		pbRoaming			out: TRUE: roaming, FALSE: home network
 * 
 * Return:
 *		ESUCCESS			registration successful
 *		EAGAIN				not registered
 * 		EACCES				registration denied
 */
result_t CModemSim900r::waitGprsRegister(hr_time_t hrTimeout, boolean_t* pbRoaming)
{
	char		buffer[64];
	hr_time_t	hrInterval;
	result_t	nr, nresult = EAGAIN;
	int 		iter = 0;
	hr_time_t	hrNow = hr_time_now();
	
	while ( hr_timeout(hrNow, hrTimeout) > HR_0 )  {
		nr = execCmd1("AT+CGREG?", buffer, sizeof(buffer), HR_2SEC, HR_2SEC);
		if ( nr == ESUCCESS )  {
			int		n, stat, count;
			
			count = _tsscanf(buffer, "+CGREG: %d,%d", &n, &stat);
			if ( count == 2 )  {
				switch ( stat )  {
					case 0:		/* Searching */
					case 2:	
						break;
					
					case 1:		/* Registered */
						nresult = ESUCCESS;
						if ( pbRoaming ) *pbRoaming = FALSE;
						break;
					
					case 5:		/* Registered, roaming */
						nresult = ESUCCESS;
						if ( pbRoaming ) *pbRoaming = TRUE;
						break;
					
					case 3:		/* Denied */
						nresult = EACCES;
						break;
				}
			}
		}		
		
		if ( nresult != EAGAIN )  {
			break;
		}
		
		hrInterval = HR_1SEC;
		if ( iter >= 20 )  {
			hrInterval = HR_3SEC;
		} else if ( iter >= 10 )  {
			hrInterval = HR_2SEC;
		}
		hr_sleep(hrInterval);
	}
	
	return nresult;	
}

/*
 * Execute GPRS Attach command
 *
 * 		hrTimeout		awaiting response time
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::gprsAttach(hr_time_t hrTimeout)
{
	char		buffer[64];
	result_t	nresult = EAGAIN, nr;
	hr_time_t	hrNow = hr_time_now();
	
	while ( hr_timeout(hrNow, hrTimeout) > HR_0 )  {
		nr = execCmd1("AT+CGATT?", buffer, sizeof(buffer), HR_2SEC, HR_2SEC);
		if ( nr == ESUCCESS )  {
			int		stat, count;
			
			count = _tsscanf(buffer, "+CGATT: %d", &stat);
			if ( count == 1 )  {
				if ( stat == 1 )  {
					nresult = ESUCCESS;
					break;
				}
			}
		}
		
		nr = execCmd0("AT+CGATT=1", buffer, sizeof(buffer), HR_2SEC, HR_5SEC);
		if ( nr != ESUCCESS )  {
			hr_sleep(HR_1SEC);
		}
	}
	
	return nresult;
}

/*
 * Execute GPRS Dettach command
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::gprsDetach()
{
	char		buffer[64];
	result_t	nresult;

	nresult = execCmd0("AT+CGATT=0", buffer, sizeof(buffer), HR_2SEC, HR_5SEC);	
	return nresult;
}

#define BEARER_PROFILE_CID			1

/*
 * Open GPRS session for IP services (HTTP)
 *
 * 		strApn			access point url
 * 		strUser			user name
 * 		strPass			user password
 * 		hrTimeout		open maximum time (19, 38, 57, ... secs)
 * 		strIpLocal		obtained local IP address (buffer of 12 bytes long) (may be 0) [OUT]
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::gprsOpenIpSession(const char* strApn, const char* strUser,
									const char* strPass, hr_time_t hrTimeout,
									char* strIpLocal)
{
	char		buffer[128];
	hr_time_t	hrNow;
	result_t	nresult;

	shell_assert(strApn);
	shell_assert(strUser);
	shell_assert(strPass);

	/*
	 * Setup session parameters
	 */
	_tsnprintf(buffer, sizeof(buffer), "AT+SAPBR=3,%d,\"CONTYPE\",\"GPRS\"",
			 BEARER_PROFILE_CID);
	nresult = execCmd0(buffer, buffer, sizeof(buffer), HR_2SEC, HR_2SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem] failed to set gprs session contype, result: %d\n", nresult);
		return nresult;
	}

	_tsnprintf(buffer, sizeof(buffer), "AT+SAPBR=3,%d,\"APN\",\"%s\"",
			 BEARER_PROFILE_CID, strApn);
	nresult = execCmd0(buffer, buffer, sizeof(buffer), HR_2SEC, HR_2SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem] failed to set gprs session apn, result: %d\n", nresult);
		return nresult;
	}

	_tsnprintf(buffer, sizeof(buffer), "AT+SAPBR=3,%d,\"USER\",\"%s\"",
			 BEARER_PROFILE_CID, strUser);
	nresult = execCmd0(buffer, buffer, sizeof(buffer), HR_2SEC, HR_2SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem] failed to set gprs session user, result: %d\n", nresult);
		return nresult;
	}

	_tsnprintf(buffer, sizeof(buffer), "AT+SAPBR=3,%d,\"PWD\",\"%s\"",
			 BEARER_PROFILE_CID, strPass);
	nresult = execCmd0(buffer, buffer, sizeof(buffer), HR_2SEC, HR_2SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem] failed to set gprs session password, result: %d\n", nresult);
		return nresult;
	}

	/*
	 * Start session
	 */
	nresult = ESUCCESS;
	hrNow = hr_time_now();

	while ( hr_timeout(hrNow, hrTimeout) > HR_0 ) {
		_tsnprintf(buffer, sizeof(buffer), "AT+SAPBR=1,%d", BEARER_PROFILE_CID);
		nresult = execCmd0(buffer, buffer, sizeof(buffer), HR_2SEC, HR_15SEC);
		if ( nresult == ESUCCESS ) {
			break;
		}

		hr_sleep(HR_3SEC);
	}

	/*
	 * Parse local IP
	 */
	if ( nresult == ESUCCESS ) {
		_tsnprintf(buffer, sizeof(buffer), "AT+SAPBR=2,%d", BEARER_PROFILE_CID);
		nresult = execCmd1(buffer, buffer, sizeof(buffer), HR_2SEC, HR_5SEC);
		if ( nresult == ESUCCESS )  {
			char	ip[128], *p;
			int		count, cid, status;
			size_t	len;

			count = _tsscanf(buffer, "+SAPBR: %d,%d,\"%s", &cid, &status, ip);
			if ( count == 3 )  {
				len = _tstrlen(ip);
				p = ip+len-1;
				while ( p >= ip && *p == '"' )  *p++='\0';

				if ( strIpLocal )  {
					copyString(strIpLocal, ip, 16);
				}
			}
			else {
				nresult = EINVAL;
				log_error(L_MODEM, "[modem(%s)] failed to parse GPRS local ip, count=%d\n",
								getName(), count);
			}
		}

		if ( nresult != ESUCCESS ) {
			gprsCloseIpSession();
		}
	}
	else {
		log_error(L_MODEM, "[modem(%s)] failed to start gprs ip session, result: %d\n",
						getName(), nresult);
	}

	return nresult;
}

/*
 * Close GPRS session for IP services
 *
 * Result: ESUCCESS, ...
 *
 * Note: Recommended 5 secs delay after closing
 */
result_t CModemSim900r::gprsCloseIpSession()
{
	char		buffer[128];
	result_t	nresult;

	_tsnprintf(buffer, sizeof(buffer), "AT+SAPBR=0,%d", BEARER_PROFILE_CID);
	nresult = execCmd0(buffer, buffer, sizeof(buffer), HR_2SEC, HR_10SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem] gprs ip session close failed, result: %d\n", nresult);
	}

	return nresult;
}

/*
 * Open HTTP Session
 *
 * 		hrTimeout		maximum timeout (10 or more seconds recommended)
 *
 * Result: ESUCCESS, ...
 */
result_t CModemSim900r::httpOpenSession(hr_time_t hrTimeout)
{
	char 		buffer[128];
	result_t	nresult;

	nresult = execCmd0("AT+HTTPINIT", buffer, sizeof(buffer), HR_2SEC, hrTimeout);
	if ( nresult != ESUCCESS ) {
		log_error(L_MODEM, "[modem] failed to init HTTP session, result: %d\n", nresult);
		return nresult;
	}

	_tsnprintf(buffer, sizeof(buffer), "AT+HTTPPARA=\"CID\",\"%d\"", BEARER_PROFILE_CID);
	nresult = execCmd0(buffer, buffer, sizeof(buffer), HR_2SEC, HR_5SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem] failed to set HTTP CID, result: %d\n", nresult);
		httpCloseSession();
		hr_sleep(HR_5SEC);
	}

	return nresult;
}

/*
 * Close HTTP Session
 *
 * Result: ESUCCESS, ...
 */
result_t CModemSim900r::httpCloseSession()
{
	char 		buffer[128];
	result_t	nresult;

	nresult = execCmd0("AT+HTTPTERM", buffer, sizeof(buffer), HR_2SEC, HR_5SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem] failed to close HTTP session, result: %d\n", nresult);
	}

	return nresult;
}

/*
 * Execute GET request
 *
 * 		strUrl			URL string
 * 		hrTimeout		maximum awaiting response time (10 or more seconds is recommended)
 * 		pLength			received response length, bytes [OUT]
 * 		pHttpCode		response HTTP code (on success) [OUT]
 *
 * Return:
 * 		ESUCCESS		success, *pLength contains response size in bytes
 * 		EINVAL			failed, response http code is not 200
 * 		ETIMEDOUT		the response wasn't received in time (there are other possible reasons)
 * 		...
 */
result_t CModemSim900r::httpGetAction(const char* strUrl, hr_time_t hrTimeout,
									  size_t* pLength, int* pHttpCode)
{
	char* 		pBuffer;
	size_t		size;
	hr_time_t	hrNow;
	result_t	nresult, nr;

	shell_assert(pLength);

	size = _tstrlen(strUrl)+ 32;
	size = MAX(MODEM_UNSOLICITED_BUFFER, size);
	pBuffer = (char*)alloca(size);

	_tsnprintf(pBuffer, size, "AT+HTTPPARA=\"URL\",\"%s\"", strUrl);
	nresult = execCmd0(pBuffer, pBuffer, size, HR_2SEC, HR_5SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem] failed to set HTTP URL, result: %d\n", nresult);
		return nresult;
	}

	/*
	 * Start GET request and wait unsolicited response
	 */
	nresult = execCmd0("AT+HTTPACTION=0", pBuffer, size, HR_2SEC, HR_5SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem] failed to start HTTP GET action, result: %d\n", nresult);
		return nresult;
	}

	nresult = ETIMEDOUT;
	hrNow = hr_time_now();

	while( hr_timeout(hrNow, hrTimeout) > HR_0 )  {
		nr = recvResp(pBuffer, size, HR_2SEC);
		if ( nr == ESUCCESS )  {
			int 	count, method, status, length;

			count = _tsscanf(pBuffer, "\r\n+HTTPACTION: %d,%d,%d",
					   &method, &status, &length);
			if ( count == 3 )  {
				if ( status == 200 )  {
					*pLength = (size_t)length;
					if ( pHttpCode )  {
						*pHttpCode = status;
					}
					nresult = ESUCCESS;
				}
				else {
					if ( pHttpCode ) {
						*pHttpCode = status;
						nresult = ESUCCESS;
					}
					else {
						log_debug(L_MODEM, "[modem] wrong http code %d received\n", status);
						nresult = EINVAL;
					}
				}
				break;
			}
		}
	}

	return nresult;
}

/*
 * Read response of GET request
 *
 * 		pBuffer			output data buffer
 * 		pSize			IN: size to read, OUT: actually read bytes
 * 		offset			read offset
 * 		hrTimeout		maximum time (30 or mode seconds is recommended)
 *
 * Return: ESUCCESS, ETIMEDOUT
 *
 * Format:
 *		send: AT+HTTPREAD=<offset>,<length>
 *		recv: +HTTPREAD: <real_length>
 *		recv: ... data ...
 *		recv: OK
 */
result_t CModemSim900r::httpRead(void* pBuffer, size_t* pSize, size_t offset,
								 hr_time_t hrTimeout)
{
	char 		buffer[128];
	size_t		size, realSize, readSize;
	int 		count;
	hr_time_t	hrNow;
	result_t	nresult;
	
	shell_assert(pBuffer);
	shell_assert(pSize);
	
	size = *pSize;
	if ( !size )  {
		return ESUCCESS;
	}
	
	_tsnprintf(buffer, sizeof(buffer), "AT+HTTPREAD=%zu,%zu", offset, size);
	nresult = sendCmd(buffer, HR_5SEC);
	if ( nresult == ESUCCESS )  {
		/* Read real length */
		hrNow = hr_time_now();		
		nresult = recvResp(buffer, sizeof(buffer), hrTimeout);
		if ( nresult == ESUCCESS )  {
			count = sscanf(buffer, "+HTTPREAD:%zu", &realSize);
			if ( count == 1 )  {
				/* Read raw data */
				readSize = MIN(realSize, size);
				nresult = doRecvRaw(pBuffer, &readSize, hr_timeout(hrNow, hrTimeout));
				if ( nresult == ESUCCESS )  {					
					/* Read trailing 'OK' */
					nresult = recvResp(buffer, sizeof(buffer), HR_5SEC);
					if ( nresult == ESUCCESS )  {
						*pSize = readSize;
					}
					else {
						log_error(L_MODEM, "[modem] failed to receive HTTP trailing OK, result: %d\n",
									nresult);
					}
				}
				else {
					log_error(L_MODEM, "[modem] failed to receive HTTP raw data of %zu bytes, result: %d\n",
									readSize, nresult);
				}
			}
			else {
				log_error(L_MODEM, "[modem] failed to parse read HTTP data length, count=%d\n", count);
				nresult = EINVAL;
			}
		}
		else {
			log_error(L_MODEM, "[modem] failed to receive http read response, result: %d\n", nresult);
		}
	}
	else {
		log_error(L_MODEM, "[modem] failed to send http read request, result: %d\n", nresult);
	}
	
	return nresult;
}

/*
 * Get RTC Clock from the modem
 *
 * 		pTime		current value, UNIX timestamp
 *
 * Return: ESUCCESS, ...
 *
 * Response format:
 *      +CCLK: "yy/MM/dd,hh:mm:ssxzz"
 *          yy:     year, last two digits (2010 => 10)
 *          MM:     month, 1..12
 *          dd:     day, 1..31
 *          hh:     hours, 00..23
 *          mm:     minutes, 00..59
 *          ss:     seconds, 00..59
 *          x:      + or -
 *          zz:     time zone, quarters of an hour, -47..+47
 */
result_t CModemSim900r::getClock(hr_time_t* pTime, hr_time_t* pTimestamp)
{
	char		buffer[128];
	result_t	nresult;

	nresult = execCmd1("AT+CCLK?", buffer, sizeof(buffer), HR_2SEC, HR_4SEC);
	if ( nresult == ESUCCESS )  {
		size_t		len = _tstrlen(buffer);
		struct tm   tm;
		time_t      modemTime;
		int         year, month, day, hours, minutes, seconds, zone, n;

		*pTimestamp = hr_time_now();

		if ( len > 7 && _tmemcmp(buffer, "+CCLK: ", 7) == 0 )  {
			n = _tsscanf(&buffer[8], "%d/%d/%d,%d:%d:%d%d",
					   &year, &month, &day, &hours, &minutes, &seconds, &zone);
			if ( n == 7 ) {
				_tbzero_object(tm);

				tm.tm_sec = seconds;
				tm.tm_min = minutes;
				tm.tm_hour = hours;
				tm.tm_mday = day;
				tm.tm_mon = month-1;
				tm.tm_year = year >= 70 ? year : (year+100);

				modemTime = timegm(&tm);
				if ( modemTime >= 0 )  {
					modemTime = modemTime - zone*(15*60);	/* Convert to GMT */
					*pTime = SECONDS_TO_HR_TIME(modemTime);
				}
				else {
					log_error(L_MODEM, "[modem] invalid clock value '%s'\n", buffer);
					nresult = EINVAL;
				}
			}
			else {
				log_error(L_MODEM, "[modem] unrecognised clock reply format '%s'\n", buffer);
				nresult = EINVAL;
			}
		}
		else {
			log_error(L_MODEM, "[modem] unrecognised clock reply '%s'\n", buffer);
			nresult = EINVAL;
		}
	}
	else {
		log_error(L_MODEM, "[modem] clock command failed, result %d\n", nresult);
	}

	return nresult;
}

/*
 * Set modem real-time clock
 *
 * 		nTime		UTC time
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::setClock(time_t nTime)
{
	time_t      nTimeToSet = nTime != (time_t)-1 ? nTime : time(NULL);
	struct tm   tm;
	char        cmd[128], buffer[128];
	result_t	nresult;

	gmtime_r(&nTimeToSet, &tm);
	_tsnprintf(cmd, sizeof(cmd), "AT+CCLK=\"%02d/%02d/%02d,%02d:%02d:%02d+00\"",
				 (tm.tm_year+1900)%100, tm.tm_mon+1, tm.tm_mday,
				 tm.tm_hour, tm.tm_min, tm.tm_sec);

	nresult = execCmd0(cmd, buffer, sizeof(buffer), HR_2SEC, HR_6SEC);
	if ( nresult == ESUCCESS )  {
		hr_sleep(HR_3SEC);
	}

	return nresult;
}

/*
 * Enable GSM Mux mode
 *
 * 		nFrameSize		maximum frame size, 1-255
 * 		nBaudRate		speed, supported: 9600, 19200, 38400, 57600, 115200,
 * 						230400, 460800, 921600
 *
 * Return: ESUCCESS, ...
 */
result_t CModemSim900r::enableMux(int nBaudRate)
{
	char 		cmd[128], resp[128];
	int			n;
	result_t	nresult;

	static struct { int nBaud; int nCode; }	arBaudCode[] = {
		{	9600,		1	},
		{	19200,		2	},
		{	38400,		3	},
		{	57600,		4	},
		{	115200,		5	},
		{	230400,		6	},
		{	460800,		7	},
		{	921600,		8	}
	};

	n = -1;
	for(size_t i=0; i<ARRAY_SIZE(arBaudCode); i++) {
		if ( nBaudRate == arBaudCode[i].nBaud )  {
			n = arBaudCode[i].nCode;
			break;
		}
	}

	if ( n < 0 )  {
		log_error(L_MODEM, "[modem(%s)] unsupported GSM Mux speed %d\n", getName(), nBaudRate);
		return EINVAL;
	}

	_tsnprintf(cmd, sizeof(cmd), "AT+CMUX=0,0,%d,%d,%d,%d,%d,%d",
		n, MUX_N1, MUX_T1, MUX_N2, MUX_T2, MUX_T3);
	nresult = execCmd0(cmd, resp, sizeof(resp), HR_2SEC, HR_6SEC);
	if ( nresult != ESUCCESS )  {
		log_error(L_MODEM, "[modem(%s)] gsm mux failed, result: %d\n", getName(), nresult);
	}

	return nresult;
}
