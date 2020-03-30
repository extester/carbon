/*
 *  Carbon framework
 *  Main application class (UNIX)
 *
 *  Copyright (c) 2015-2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 20.06.2015 22:54:31
 *      Initial revision.
 *
 *  Revision 1.1, 14.08.2017 14:17:30
 *  	Fixed parseAppPath(), changed strPath size to PATH_MAX
 */

#include <limits.h>

#include "shell/config.h"
#include "shell/socket.h"

#include "carbon/carbon.h"
#include "carbon/application.h"
#include "carbon/net_container.h"
#if CARBON_JANSSON
#include "carbon/json.h"
#endif /* CARBON_JANSSON */


/*******************************************************************************
 * CApplication class
 */

CApplication::CApplication(const char* strName, version_t version, unsigned build,
						   	int argc, char* argv[]) :
	CModule(strName),
	CEventLoopThread("MainLoop"),
	CEventReceiver(this, strName),
	m_appVersion(version),
	m_appBuild(build),
	m_nExitCode(0),
	m_hPickupAppender(LOGGER_APPENDER_NULL),
	m_argc(argc),
	m_argv(argv)
{
	shell_assert_ex(!g_pApp, "Application object is duplicated!\n");
	if ( !g_pApp )  {
		g_pApp = this;
	}

	carbon_init();

	/* Initialise command line arguments */
	initCommandLineArgs(argc, argv);
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
#if CARBON_JANSSON
	CJsonObject::check("CJsonObject");
#endif /* CARBON_JANSSON */
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

		case EV_HUP:
			onEvHup();
			bProcessed = TRUE;
			break;

		case EV_USR1:
			onEvUsr1();
			bProcessed = TRUE;
			break;

		case EV_USR2:
			onEvUsr2();
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
	/* Tracing logs are disabled by default */
	logger_disable(LT_TRACE|L_ALL);

	logger_disable(LT_ALL|L_EV_TRACE_EVENT);
	logger_disable(LT_ALL|L_EV_TRACE_TIMER);
	logger_disable(LT_ALL|L_NETCONN_IO);
	logger_disable(LT_ALL|L_NETSERV_IO);
	logger_disable(LT_ALL|L_NETCLI_IO);
}

/*
 * Add a pickup appender to the current logger
 *
 * 		strFile		pickup appender filename
 */
void CApplication::initLoggerPickupAppender(const char* strFile)
{
	appender_handle_t	hAppender;

	shell_assert(m_hPickupAppender == LOGGER_APPENDER_NULL);

	hAppender = logger_addPickupAppender(strFile);
	if ( hAppender != LOGGER_APPENDER_NULL )  {
		m_hPickupAppender = hAppender;
	}
}

/*
 * Get the next available logger block of log strings
 * from the pickup appender
 *
 * 		pBuffer		output buffer
 * 		pSize		buffer size, bytes [in/out]
 *
 * Return:
 * 		ESUCCESS	got a block of logs (MUST be confirmed by loggerBlockDone())
 * 		EBADF		pickup appender is not exists
 */
result_t CApplication::getLoggerBlock(void* pBuffer, size_t* pSize)
{
	result_t	nresult = EBADF;

	if ( m_hPickupAppender != LOGGER_APPENDER_NULL )  {
		nresult = logger_getPickupAppenderBlock(m_hPickupAppender, pBuffer, pSize);
	}

	return nresult;
}

/*
 * Confirm that a last logger block has been successfully processed
 * and may be discarded.
 *
 * 		size		size of block, bytes
 */
void CApplication::loggerBlockDone(size_t size)
{

}

void CApplication::parseAppPath(const char* strArgv0)
{
	char    *strTmp, *s, strPath[PATH_MAX];

	shell_assert(strArgv0);

	s = realpath(strArgv0, strPath);
	if ( s != NULL ) {
		strTmp = _tstrrchr(strPath, '/');
		if ( strTmp != NULL )  {
			*strTmp = '\0';			/* Remove program name */
			copyString(m_strAppPath, strPath, sizeof(m_strAppPath));
		}
		else  {
			copyString(m_strAppPath, "/tmp", sizeof(m_strAppPath));
			log_error(L_GEN, "[app] invalid argv0 (%s), can't determine application path, "
					"use default '%s'\n", strArgv0, m_strAppPath);
		}
	}
	else {
		/* Invalid path string */
		copyString(m_strAppPath, "/tmp", sizeof(m_strAppPath));
		log_error(L_GEN, "[app] invalid argv0 (%s), can't determine application path, "
				"use default, '%s'\n", strArgv0, m_strAppPath);
	}
}


void CApplication::initCommandLineArgs(int argc, char* argv[])
{
	copyString(m_strAppPath, "/tmp", sizeof(m_strAppPath));

	if ( argc < 1 || argv[0] == 0 )  {
		log_error(L_GEN, "[app] invalid argc/argv parameters, application path set to %s\n",
				  m_strAppPath);
		return;
	}

	/* Setup application path */
	parseAppPath(argv[0]);
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
	deleteEventAll();
//	logger_terminate();
	m_hPickupAppender = LOGGER_APPENDER_NULL;
}


/*
 * Application exec loop
 */
int CApplication::run()
{
	result_t	nresult;

	log_info(L_GEN, "~~~ Starting %s, version %d.%d ~~~\n",
			 CModule::getName(), VERSION_MAJOR(m_appVersion), VERSION_MINOR(m_appVersion));
	log_info(L_GEN, "~~~ Build %u, %s, %s ~~~\n", m_appBuild, __DATE__, __TIME__);

	setExitCode(0);

	nresult = init();
	if ( nresult == ESUCCESS ) {
		CEvent*		pEvent;
		pEvent = new CEvent(EV_START, EVENT_MULTICAST, (PPARAM)0, (NPARAM)0);
		appSendMulticastEvent(pEvent, appMainLoop());

		nresult = CEventLoopThread::start();
		if ( nresult == ESUCCESS ) {
			m_signalServer.run();

			log_info(L_GEN, "~~~ Stopping %s (exit code %d) ~~~\n", CModule::getName(), getExitCode());
			CEventLoopThread::stop();
		}
		else {
			setExitCode(nresult);
		}
		terminate();
	}
	else {
		log_error(L_GEN, "~~~ Failure on startup %s, error %d ~~~\n", CModule::getName(), nresult);
		setExitCode(nresult);
	}

	return getExitCode();
}

void CApplication::stopApplication(int nExitCode)
{
	setExitCode(nExitCode);
	m_signalServer.stop();
}

/*******************************************************************************
 * Debugging support
 */

#if CARBON_DEBUG_DUMP

void CApplication::dump(const char* strPref) const
{
	shell_unused(strPref);
}

#endif /* CARBON_DEBUG_DUMP */

/*******************************************************************************/

CApplication* g_pApp = 0;
