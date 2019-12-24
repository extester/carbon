/*
 *  Carbon framework
 *  Main application class for STM32 machine (STM32)
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 16.02.2017 14:43:44
 *      Initial revision.
 */

#ifndef __CARBON_APPLICATION_H_INCLUDED__
#define __CARBON_APPLICATION_H_INCLUDED__

#include "shell/config.h"

#include "carbon/event/eventloop.h"
#include "carbon/module.h"


class CApplication : public CModule, public CEventLoopThread, public CEventReceiver
{
	private:
		version_t			m_appVersion;
		int					m_nExitCode;

	public:
		CApplication(const char* strName, version_t version, int argc = 0, char* argv[] = NULL);
		virtual ~CApplication();

	public:
		int getExitCode() const { return m_nExitCode; }
		virtual version_t getVersion() const { return m_appVersion; }
		CEventLoop* getMainLoop() { return this; }

		int run();
		void stopApplication(int nResultCode);

	protected:
		virtual boolean_t processEvent(CEvent* pEvent);

		virtual void initEvent();
		virtual void initLogger();

		void onEvQuit(int nExitCode) { /*setExitCode(nExitCode); m_signalServer.stop();*/ }

		virtual result_t init();
		virtual void terminate();

	private:
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
