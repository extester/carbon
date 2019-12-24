/*
 *  Carbon framework
 *  Global Module class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 30.03.2015 15:00:01
 *      Initial revision.
 */

#ifndef __CARBON_MODULE_H_INCLUDED__
#define __CARBON_MODULE_H_INCLUDED__

#include "shell/config.h"
#include "shell/object.h"
#include "shell/counter.h"


class CModule : public CObject
{
	protected:
		CModule();
		CModule(const char* strName);

	public:
		virtual ~CModule();

	public:
		virtual result_t init() { return ESUCCESS; }
		virtual void terminate() {}

	protected:
		/* Statistic functions */
		virtual size_t getStatSize() const { return 0; };
		virtual void getStat(void* pBuffer, size_t nSize) const  {}
		virtual void resetStat() {}

#if CARBON_DEBUG_DUMP
	public:
		virtual void dump(const char* strPref = "") const = 0;
#endif /* CARBON_DEBUG_DUMP */
};

extern result_t appInitModules(CModule* arModule[], size_t count);
extern void appTerminateModules(CModule* arModule[], size_t count);

#endif /* __CARBON_MODULE_H_INCLUDED__ */
