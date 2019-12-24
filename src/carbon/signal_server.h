/*
 *  Carbon framework
 *  UNIX Signal server
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 01.06.2015 14:44:51
 *      Initial revision.
 */

#ifndef __CARBON_SIGNAL_SERVER_H_INCLUDED__
#define __CARBON_SIGNAL_SERVER_H_INCLUDED__

#include <pthread.h>

#include "shell/object.h"
#include "shell/atomic.h"

#include "carbon/event.h"

class CSignalServer : public CObject
{
	private:
		pthread_t					m_threadId;
		atomic_t					m_bDone;
		static volatile uint32_t	m_bmpSignal;

	public:
		CSignalServer();
		virtual ~CSignalServer();

	public:
		void run();
		void stop();

	private:
		result_t installHandlers();
		void removeHandlers();

		void sendEvent(event_type_t event, NPARAM nparam) const;

		result_t setHandler(int signum, void (*handler)(int));
		static void fatalHandler(int signum);
		static void userHandler(int signum);
};

#endif /* __CARBON_SIGNAL_SERVER_H_INCLUDED__ */
