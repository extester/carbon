/*
 *  Carbon framework
 *  UNIX Signal server
 *
 *  Copyright (c) 2015-2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 01.06.2015 14:54:50
 *      Initial revision.
 *
 *  Revision 1.1, 28.06.2015 14:07:57
 *  	Fixed stopHandler(): stop only than processed in main thread.
 */
/*
 * Processing signals:
 *
 * 		SIGINT								send EV_QUIT to the application event loop
 * 		SIGTERM								send EV_QUIT to the application event loop
 * 		SIGHUP								send EV_HUP to the application event loop
 * 		SIGUSR1								send EV_USR1 to the application event loop
 * 		SIGUSR2								send EV_USR1 to the application event loop
 *
 * 		SIGQUIT								used internally for thread termination
 * 		SIGPIPE, SIGSERV, SIGBUS, SIGILL	dump stack and exit
 */

#include <signal.h>

#include "shell/shell.h"
#include "shell/debug.h"

#include "carbon/application.h"
#include "carbon/signal_server.h"

static CSignalServer* g_pSignalServer = 0;

/*******************************************************************************
 * CSignalServer class
 */

volatile uint32_t CSignalServer::m_bmpSignal = 0;


CSignalServer::CSignalServer() :
	CObject("SignalServer")
{
	shell_assert_ex(!g_pSignalServer, "Signal server object is duplicated!\n");

	if ( !g_pSignalServer ) {
		m_threadId = pthread_self();
		atomic_set(&m_bDone, FALSE);
		m_bmpSignal = 0;

		g_pSignalServer = this;
		installHandlers();
	}
}

CSignalServer::~CSignalServer()
{
	if ( g_pSignalServer )  {
		removeHandlers();
		g_pSignalServer = 0;
	}
}

/*
 * Fatal errors handler, terminate an application
 *
 * 		signum		signal number
 */
void CSignalServer::fatalHandler(int signum)
{
	log_dump("Fatal error, got a signal %d\n", signum);
	log_dump("Stack trace:\n");
	printBacktrace();
	exit(0xff);
}

/*
 * Override user defined signal handler
 *
 * 		signum		signal number
 */
void CSignalServer::userHandler(int signum)
{
	if ( g_pSignalServer != 0 && pthread_self() == g_pSignalServer->m_threadId ) {
		g_pSignalServer->m_bmpSignal |= 1<<signum;
	}
}

/*
 * Setup a signal handler
 *
 * 		signum		signal number SIGXXX
 * 		handler		signal handler
 *
 * Return: ESUCCESS, ...
 */
result_t CSignalServer::setHandler(int signum, void (*handler)(int))
{
	struct sigaction	sa;
	result_t			nresult = ESUCCESS;

	_tbzero_object(sa);
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	if ( sigaction(signum, &sa, NULL) == -1 )  {
		nresult = errno;
		log_error(L_GEN, "[signal_server] failed to setup signal handler %d, result: %d\n",
				  signum, nresult);
	}

	return nresult;
}

/*
 * Install all interested signal handlers
 *
 * Return:
 * 		ESUCCESS		all handlers are installed
 * 		...				one or more signal handlers are not installed
 */
result_t CSignalServer::installHandlers()
{
	result_t	nresult, nr;

	/*
	 * Stopping signals
	 */
	nresult = setHandler(SIGTERM, userHandler);

	nr = setHandler(SIGINT, userHandler);
	nresult_join(nresult, nr);

	/*
	 * User handled signals
	 */
	nr = setHandler(SIGHUP, userHandler);
	nresult_join(nresult, nr);

	nr = setHandler(SIGUSR1, userHandler);
	nresult_join(nresult, nr);

	nr = setHandler(SIGUSR2, userHandler);
	nresult_join(nresult, nr);

	/*
	 * Fatal signals
	 */
	nr = setHandler(SIGPIPE, fatalHandler);
	nresult_join(nresult, nr);

	nr = setHandler(SIGSEGV, fatalHandler);
	nresult_join(nresult, nr);

	nr = setHandler(SIGBUS, fatalHandler);
	nresult_join(nresult, nr);

	nr = setHandler(SIGILL, fatalHandler);
	nresult_join(nresult, nr);

	/*
	 * SIGQUIT is used for thread termination
	 *
	 * Handler must be the userHandler() to allow
	 * TCP log appender terminate properly.
	 */
	nr = setHandler(SIGQUIT, userHandler);
	nresult_join(nresult, nr);

	return nresult;
}

/*
 * Restore used signal handlers to the default handlers
 */
void CSignalServer::removeHandlers()
{
	setHandler(SIGILL, SIG_DFL);
	setHandler(SIGBUS, SIG_DFL);
	setHandler(SIGSEGV, SIG_DFL);
	setHandler(SIGPIPE, SIG_DFL);
	setHandler(SIGUSR2, SIG_DFL);
	setHandler(SIGUSR1, SIG_DFL);
	setHandler(SIGHUP, SIG_DFL);
	setHandler(SIGTERM, SIG_DFL);
	setHandler(SIGINT, SIG_DFL);
	setHandler(SIGQUIT, SIG_DFL);
}

void CSignalServer::sendEvent(event_type_t event, NPARAM nparam) const
{
	CEventReceiver*	pReceiver = appApplication;
	CEvent*			pEvent;

	if ( pReceiver )  {
		pEvent = new CEvent(event, pReceiver, NULL, nparam);
		appSendEvent(pEvent);
	}
}

/*
 * Run a server until SIGTERM/SIGINT is received
 */
void CSignalServer::run()
{
	while ( atomic_get(&m_bDone) == FALSE ) {
		sleep_s(10);

		if ( m_bmpSignal != 0 ) {
			uint32_t bmpSignal = 0;

			while ( !__sync_bool_compare_and_swap(&m_bmpSignal, bmpSignal, 0) ) {
				bmpSignal = m_bmpSignal;
			}

			if ( bmpSignal&(1<<SIGHUP) )  {
				log_debug(L_GEN_FL, "[signal_server] signal SIGHUP detected..\n");
				g_pSignalServer->sendEvent(EV_HUP, 0);
			} else
			if ( bmpSignal&(1<<SIGUSR1) )  {
				log_debug(L_GEN_FL, "[signal_server] signal SIGUSR1 detected..\n");
				g_pSignalServer->sendEvent(EV_USR1, 0);
			} else
			if ( bmpSignal&(1<<SIGUSR2) )  {
				log_debug(L_GEN_FL, "[signal_server] signal SIGUSR2 detected..\n");
				g_pSignalServer->sendEvent(EV_USR2, 0);
			} else
			if ( bmpSignal&(1<<SIGINT) ) {
				g_pSignalServer->sendEvent(EV_QUIT, 0);
			} else
			if ( bmpSignal&(1<<SIGTERM) )  {
				g_pSignalServer->sendEvent(EV_QUIT, 0);
			}
		}
	}
}

/*
 * Stop signal server
 */
void CSignalServer::stop()
{
	if ( g_pSignalServer ) {
		atomic_set(&g_pSignalServer->m_bDone, TRUE);
	}

	pthread_kill(m_threadId, SIGQUIT);
}
