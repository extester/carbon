/*
 *  Carbon framework
 *  Multi packets I/O exchange
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.04.2015 12:34:32
 *      Initial revision.
 */

#include <poll.h>

#include "shell/shell.h"
#include "shell/memory.h"
#include "shell/file.h"
#include "shell/logger.h"

#include "carbon/multi_io.h"


/*******************************************************************************
 * General net container multi I/O class
 */

/*
 * Reset breaker file descriptor
 */
void CMultiIo::resetBreaker()
{

    shell_assert(m_hBreaker);
    if ( m_hBreaker )  {
		CFile	file;
        char    buf[32];
		size_t	size;

		file.attachHandle(m_hBreaker);
		do {
			size = sizeof(buf);

		} while ( file.read(buf, &size) == ESUCCESS );
    }
}

void CMultiIo::itemCompleted(multi_io_t* pMio, struct pollfd* pPoll, result_t nresult)
{
	pMio->socket.close();
	pMio->nresult = nresult;
	pMio->stage = CMultiIo::stageCompleted;

	pPoll->fd = -1;
}

result_t CMultiIo::doSend(multi_io_t* pMio)
{
	result_t	nresult;

	shell_assert(pMio->pSendContainer);

	nresult = pMio->pSendContainer->send(pMio->socket, &pMio->sendOffset);
	return nresult;
}

result_t CMultiIo::doReceive(multi_io_t* pMio)
{
	result_t	nresult;

	shell_assert(pMio->pRecvContainer);

	nresult = pMio->pRecvContainer->receive(pMio->socket);
	return nresult;
}

/*
 * Perform multi info-packet exchange
 *
 * 		arMio			info-packet array
 * 		count			info-packets in the array
 * 		hrTimeout		maximum timeout for all packets
 * 		strTitle		exchange name (for logging)
 * 		hBreaker		opened file descriptor to cancel I/O, may be 0
 *
 * Return:
 * 		ESUCCESS		success (individual info-packet result in array)
 * 		ETIMEDOUT		some packets were not exchanged in the timeout
 * 		ECANCEL			canceled by user/interrupt
 * 		...
 */
result_t CMultiIo::multiIo(multi_io_t* arMio, int count, hr_time_t hrTimeout)
{
	struct pollfd	*arPoll, *pPoll;;
	int				i, nPending, msTimeout;
	hr_time_t		hrStart;
	result_t		nresult, nr;
	multi_io_t*		pMio;

	/*
	 * Sanity checks
	 */
	if ( count < 1 )  {
		return ESUCCESS;
	}

	if ( count > MIO_PACKETS_MAX )  {
		log_error(L_MIO, "[multi_io] %s: too many packets (%d) in I/O, limit %d\n",
						m_strTitle, count, MIO_PACKETS_MAX);
		return EINVAL;
	}

	for(i=0; i<count; i++)  {
		arMio[i].nresult = ESUCCESS;
		arMio[i].socket.close();
		arMio[i].sendOffset = 0;
	}

	if ( hrTimeout == HR_0 )  {
		log_error(L_MIO, "[multi_io] %s: zero timeout specified\n", m_strTitle);
		shell_assert(hrTimeout > HR_0);
		return ETIMEDOUT;
	}

	arPoll = (struct pollfd*)memAlloca((count+1)*sizeof(struct pollfd));
	_tbzero(arPoll, (count+1)*sizeof(struct pollfd));

	/*
	 * Connecting
	 */
	nPending = 0;
	nresult = ESUCCESS;

	for(i=0; i<count && nresult == ESUCCESS; i++)  {
		pMio = &arMio[i];
		pPoll = &arPoll[i];

		nr = pMio->socket.connect(pMio->netAddr);
		switch( nr )  {
			case EINPROGRESS:			/* Connecting */
				log_debug(L_NET_FL, "[mio(%i)] %s: connecting to %s in progress...\n",
							i, m_strTitle, (const char*)pMio->netAddr);
				pMio->stage = CMultiIo::stageConnect;
				nPending++;
				pPoll->fd = pMio->socket.getHandle();
				pPoll->events = POLLIN|POLLPRI|POLLOUT;
				break;

			case ESUCCESS:				/* Connected */
				log_debug(L_NET_FL, "[mio(%i)] %s: connected to %s\n",
							i, m_strTitle, (const char*)pMio->netAddr);
				if ( pMio->options&CMultiIo::mioSend )  {
					log_debug(L_MIO, "[mio(%i)] %s: start sending to %s\n",
								i, m_strTitle, (const char*)pMio->netAddr);
					pMio->stage = CMultiIo::stageSend;
					nPending++;
					pPoll->fd = pMio->socket.getHandle();
					pPoll->events = POLLOUT;
				} else
				if ( pMio->options&CMultiIo::mioReceive )  {
					log_debug(L_NET_FL, "[mio(%i)] %s: start receiving to %s\n",
								i, m_strTitle, (const char*)pMio->netAddr);
					pMio->stage = CMultiIo::stageReceive;
					nPending++;
					pPoll->fd = pMio->socket.getHandle();
					pPoll->events = POLLIN|POLLPRI;
				} else {
					itemCompleted(pMio, pPoll, ESUCCESS);
					log_warning(L_MIO, "[multi_io] %s: no I/O operation specified for "
							"connection %s, ignored\n", m_strTitle, (const char*)pMio->netAddr);
				}
				break;

			case EINTR:
			case ECANCELED:
				log_debug(L_NET_FL, "[mio] mio[%i] connecting to %s canceled, result: %d\n",
							i, (const char*)pMio->netAddr, nr);
				nresult = ECANCELED;
				break;

			default:
				itemCompleted(pMio, pPoll, nr);
				log_debug(L_MIO, "[multi_io] %s: failed to connect to %s, result %d\n",
							m_strTitle, (const char*)pMio->netAddr, nr);
				break;
		}
	}

	if ( m_hBreaker )  {
		arPoll[count].fd = m_hBreaker;
		arPoll[count].events = POLLIN|POLLPRI|POLLOUT;
	}

	hrStart = hr_time_now();
	while ( nresult == ESUCCESS && nPending > 0 )
	{
		msTimeout = (int)HR_TIME_TO_MILLISECONDS(hrTimeout-hr_time_get_elapsed(hrStart));
		if ( msTimeout < 1 )  {
			nresult = ETIMEDOUT;
			break;
		}

		nr = poll(arPoll, (nfds_t)count + (m_hBreaker!=FALSE), msTimeout);
		if ( nr == 0 )  {
			/* Timeout and no sockets are ready */
			nresult = ETIMEDOUT;
			break;
		}

		if ( nr < 0 )  {
			/* Polling failed */
			nresult = errno;
			log_debug(L_MIO, "[multi_io] %s: poll() failed, result %d\n",
								m_strTitle, nresult);
			break;
		}

		if ( m_hBreaker != 0 && (arPoll[count].revents&(POLLIN|POLLPRI|POLLOUT)) )  {
			/* Cancel I/O */
			resetBreaker();
			nresult = ECANCELED;
			break;
		}

		for(i=0; i<count; i++)  {
			pMio = &arMio[i];
			pPoll = &arPoll[i];

			short		revents = pPoll->revents;
			boolean_t	bCompleted = FALSE;

			switch (pMio->stage)  {
				case CMultiIo::stageConnect:		/* Item connected */
					if ( revents&(POLLIN|POLLPRI|POLLOUT) )  {
						if ( pMio->options&CMultiIo::mioSend ) {
							/*
							 * Sending
							 */
							log_debug(L_NET_FL, "[mio(%d)] %s: sending to %s...\n",
											i, m_strTitle, (const char*)pMio->netAddr);
							nr = doSend(pMio);
							if ( nr == ESUCCESS )  {
								/* Send completed */
								if ( pMio->options&CMultiIo::mioReceive )  {
									pMio->stage = CMultiIo::stageReceive;
									pPoll->events = POLLIN|POLLPRI;
								}
								else {
									itemCompleted(pMio, pPoll, ESUCCESS);
									bCompleted = TRUE;
								}
							} else
							if ( nr == EAGAIN )  {
								log_debug(L_NET_FL, "[mio(%d)] %s: sending to %s AGAIN...\n",
											i, m_strTitle, (const char*)pMio->netAddr);
								pMio->stage = CMultiIo::stageSend;
								pPoll->events = POLLOUT;
							} else {
								/* Send error */
								nr = nr == EINTR ? ECANCELED : nr;
								itemCompleted(pMio, pPoll, nr);
								bCompleted = TRUE;
								if ( nr != ECANCELED ) {
									log_error(L_MIO, "[multi_io] %s: send error %d, address %s\n",
													m_strTitle, nr, (const char*)pMio->netAddr);
								}
							}
						} else
						if ( pMio->options&CMultiIo::mioReceive )  {
							/*
							 * Receiving
							 */
							log_debug(L_NET_FL, "[mio(%d)] %s: start receiving...\n", i, m_strTitle);
							pMio->stage = CMultiIo::stageReceive;
							pPoll->events = POLLIN|POLLPRI;
						} else {
							shell_assert(FALSE);
						}
					}

					if ( revents&(POLLERR|POLLHUP|POLLNVAL) )  {
						log_error(L_MIO, "[multi_io] %s: connection to %s failed, result %d, flags: %s %s %s\n",
									m_strTitle, (const char*)pMio->netAddr, nr,
									revents&POLLERR ? "POLLERR" : "",
									revents&POLLHUP ? "POLLHUP" : "",
									revents&POLLNVAL ? "POLLNVAL" : "");

						itemCompleted(pMio, pPoll, EIO);
						bCompleted = TRUE;
					}
					break;

				case CMultiIo::stageSend:				/* Item sending */
					if ( revents&POLLOUT )  {
						/*
						 * Sending
						 */
						nr = doSend(pMio);
						if ( nr == ESUCCESS )  {
							/* Send completed */
							if ( pMio->options&CMultiIo::mioReceive )  {
								pMio->stage = CMultiIo::stageReceive;
								pPoll->events = POLLIN|POLLPRI;
							}
							else {
								itemCompleted(pMio, pPoll, ESUCCESS);
								bCompleted = TRUE;
							}
						} else
						if ( nr == EAGAIN )  {
							pPoll->events = POLLOUT;
						} else {
							/* Send error */
							nr = nr == EINTR ? ECANCELED : nr;
							itemCompleted(pMio, pPoll, nr);
							bCompleted = TRUE;
							if ( nr != ECANCELED )  {
								log_error(L_MIO, "[multi_io] %s: send error %d, address %s\n",
												m_strTitle, nr, (const char*)pMio->netAddr);
							}
						}
					}

					if ( revents&(POLLERR|POLLHUP|POLLNVAL) )  {
						log_error(L_MIO, "[multi_io] %s: sending to %s failed, result %d, flags: %s %s %s\n",
									m_strTitle, (const char*)pMio->netAddr, nr,
									revents&POLLERR ? "POLLERR" : "",
									revents&POLLHUP ? "POLLHUP" : "",
									revents&POLLNVAL ? "POLLNVAL" : "");

						itemCompleted(pMio, pPoll, EIO);
						bCompleted = TRUE;
					}
					break;

				case CMultiIo::stageReceive:				/* Item receiving */
					if ( revents&(POLLIN|POLLPRI) )  {
						/*
						 * Receiving
						 */
						log_debug(L_NET_FL, "[mio(%d)] %s: receiving from %s...\n",
										i, m_strTitle, (const char*)pMio->netAddr);
						nr = doReceive(pMio);
						if ( nr == ESUCCESS )  {
							/* Receive completed */
							itemCompleted(pMio, pPoll, ESUCCESS);
							bCompleted = TRUE;
						} else
						if ( nr == EAGAIN )  {
							pPoll->events = POLLIN|POLLPRI;
						} else {
							/* Receive error */
							nr = nr == EINTR ? ECANCELED : nr;
							itemCompleted(pMio, pPoll, nr);
							bCompleted = TRUE;
							if ( nr != ECANCELED )  {
								log_error(L_MIO, "[multi_io] %s: receive error %d, address %s\n",
												m_strTitle, nr, (const char*)pMio->netAddr);
							}
						}
					}

					if ( revents&(POLLERR|POLLHUP|POLLNVAL) )  {
						log_error(L_MIO, "[multi_io] %s: receving from %s failed, result %d, flags: %s %s %s\n",
									m_strTitle, (const char*)pMio->netAddr, nr,
									revents&POLLERR ? "POLLERR" : "",
									revents&POLLHUP ? "POLLHUP" : "",
									revents&POLLNVAL ? "POLLNVAL" : "");

						itemCompleted(pMio, pPoll, EIO);
						bCompleted = TRUE;
					}
					break;

				case CMultiIo::stageCompleted:
					break;

				default:
					shell_assert_ex(FALSE, "invalid stage %d\n", pMio->stage);
					break;
			}

			nPending -= bCompleted != FALSE;
			shell_assert(nPending >= 0);
			sleep_s(1);
		}
	} /* while */

	for(i=0; i<count; i++)  {
		if ( arPoll[i].fd >= 0 )  {
			itemCompleted(&arMio[i], &arPoll[i], nresult);
			continue;
		}
		shell_assert(arMio[i].stage == CMultiIo::stageCompleted);
	}

	return nresult;
}
