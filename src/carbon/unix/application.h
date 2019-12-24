/*
 *  Carbon framework
 *  Main application class (UNIX)
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 20.06.2015 22:50:55
 *      Initial revision.
 */

#ifndef __CARBON_APPLICATION_H_INCLUDED__
#define __CARBON_APPLICATION_H_INCLUDED__

#include "shell/config.h"

#include "carbon/signal_server.h"
#include "carbon/event/eventloop.h"
#include "carbon/module.h"


class CApplication : public CModule, public CEventLoopThread, public CEventReceiver
{
	private:
		CSignalServer		m_signalServer;
		version_t			m_appVersion;
		unsigned			m_appBuild;
		char				m_strAppPath[96];
		int					m_nExitCode;

		appender_handle_t	m_hPickupAppender;

	protected:
		int 				m_argc;
		char**				m_argv;

	public:
		CApplication(const char* strName, version_t version, unsigned build = 1,
					 	int argc = 0, char* argv[] = NULL);
		virtual ~CApplication();

	public:
		int getExitCode() const { return m_nExitCode; }
		virtual version_t getVersion() const { return m_appVersion; }
		CEventLoop* getMainLoop() { return this; }
		const char* getAppPath()  { return m_strAppPath; }

		int run();
		void stopApplication(int nResultCode);

		result_t getLoggerBlock(void* pBuffer, size_t* pSize);
		void loggerBlockDone(size_t size);

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);

		virtual void initEvent();
		virtual void initLogger();
		void initLoggerPickupAppender(const char* strFile);

		virtual void onEvQuit(int nExitCode) { setExitCode(nExitCode); m_signalServer.stop(); }
		virtual void onEvHup() {}
		virtual void onEvUsr1() {}
		virtual void onEvUsr2() {}

		virtual result_t init();
		virtual void terminate();

	private:
		void parseAppPath(const char* strArgv0);
		void initCommandLineArgs(int argc, char* argv[]);

		void setExitCode(int nExitCode) {
			if ( m_nExitCode == 0 )  {
				m_nExitCode = nExitCode;
			}
		}

#if CARBON_DEBUG_DUMP
	public:
		virtual void dump(const char* strPref = "") const;
#endif /* CARBON_DEBUG_DUMP */
};

extern CApplication* g_pApp;

#define appApplication		g_pApp

#endif /* __CARBON_APPLICATION_H_INCLUDED__ */
