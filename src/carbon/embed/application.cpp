/*
 *  Carbon framework
 *  Main application class (STM32)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 16.02.2017 16:46:15
 *      Initial revision.
 */

#include "carbon/carbon.h"
#include "carbon/application.h"


/*******************************************************************************
 * CApplication class
 */

CApplication::CApplication(const char* strName, version_t version, int argc, char* argv[]) :
	CModule(strName),
	CEventLoopThread("MainLoop"),
	CEventReceiver(this, strName),
	m_appVersion(version),
	m_nExitCode(0)
{
	UNUSED(argc);
	UNUSED(argv);

	shell_assert_ex(!g_pApp, "Application object is duplicated!\n");
	if ( !g_pApp )  {
		g_pApp = this;
	}

	carbon_init();
}

CApplication::~CApplication()
{
	g_pApp = 0;
	shell_assert(!CEventLoopThread::isRunning());


#if CARBON_DEBUG_TRACK_OBJECT
	/*
	 * Check for object leaks
	 */
	CEvent::check("CEvent");
	CTimer::check("CTimer");
	CNetContainer::check("CNetContainer");
	CSocketRef::check("CSocketRef");
#endif /* CARBON_DEBUG_TRACK_OBJECT */

	carbon_terminate();
}

/*
 * Application wide event processing (overridable)
 *
 *      pEvent      event object to process
 *
 * Return:
 *      TRUE        event processed
 *      FALSE       event is not processed
 */
/*
 * Application wide event processing (overridable)
 */
boolean_t CApplication::processEvent(CEvent* pEvent)
{
	boolean_t	bProcessed = FALSE;

	switch ( pEvent->getType() )  {
		case EV_QUIT:
			/*
			 * Quitting application, NPARAM: exit code
			 */
			onEvQuit((int)pEvent->getnParam());
			bProcessed = TRUE;
			break;
	}

	return bProcessed;
}

/*
 * Initialise program-specific events (event strucbg table, etc)
 */
void CApplication::initEvent()
{
	/* Initialise event string table */
	registerCarbonEventString();
}

/*
 * Logger initalisation
 */
void CApplication::initLogger()
{
	logger_disable(LT_ALL|L_EV_TRACE_EVENT);
	logger_disable(LT_ALL|L_EV_TRACE_TIMER);

	logger_disable(LT_ALL|L_ICMP_FL);
	logger_disable(LT_ALL|L_NET_FL);

	logger_disable(LT_ALL|L_GEN_FL);
	logger_disable(LT_ALL|L_SOCKET_FL);

	logger_disable(LT_ALL|L_NETCONN_IO);
	logger_disable(LT_ALL|L_NETCONN_FL);

	logger_disable(LT_ALL|L_NETSERV_IO);
	logger_disable(LT_ALL|L_NETSERV_FL);
	logger_disable(LT_ALL|L_NETCLI_IO);
	logger_disable(LT_ALL|L_NETCLI_FL);

	logger_disable(LT_ALL|L_HTTP_CONT_FL);
}

/*
 * Application initialisation routine
 *
 * Return: ESUCCESS, or system result code
 */
result_t CApplication::init()
{
	result_t	nresult = ESUCCESS;

	initLogger();
	initEvent();

	return nresult;
}

/*
 * Application termination routine
 */
void CApplication::terminate()
{
//	logger_terminate();
}


/*
 * Application exec loop
 */
int CApplication::run()
{
	result_t	nresult;

	log_info(L_GEN, "~~~ Starting %s, version %d.%d (%s, %s) ~~~\n",
			 CModule::getName(), VERSION_MAJOR(m_appVersion), VERSION_MINOR(m_appVersion),
			 __DATE__, __TIME__);

	setExitCode(0);

	nresult = init();
	if ( nresult == ESUCCESS ) {
		nresult = CEventLoopThread::start();
		if ( nresult == ESUCCESS ) {
			while ( isRunning() ) {
				sleep_s(1);
			}

			log_info(L_GEN, "~~~ Stopping %s (result %d) ~~~\n", CModule::getName(), getExitCode());
			CEventLoopThread::stop();
		}
		else {
			log_error(L_GEN, "~~~ Failure on startup %s, error %d ~~~\n", CModule::getName(), nresult);
			setExitCode(nresult);
		}
		terminate();
	}

	return getExitCode();
}

void CApplication::stopApplication(int nExitCode)
{
	setExitCode(nExitCode);
	stopEventLoop();
}

/*******************************************************************************
 * Debugging support
 */

#if CARBON_DEBUG_DUMP

void CApplication::dump(const char* strPref) const
{
	UNUSED(strPref);
}

#endif /* CARBON_DEBUG_DUMP */

/*******************************************************************************/

CApplication* g_pApp = 0;
