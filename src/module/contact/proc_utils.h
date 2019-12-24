/*
 *  Carbon/Contact module
 *  UNIX 'proc' filesystem utilities
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.11.2017 10:49:39
 *      Initial revision.
 */

#ifndef __CONTACT_PROC_UTILS_H_INCLUDED__
#define __CONTACT_PROC_UTILS_H_INCLUDED__

#include <unistd.h>
#include <signal.h>

#include "shell/shell.h"
#include "shell/hr_time.h"

typedef struct
{
	char 		strName[128];			/* Executable file name (full or short) */
	char		strParams[512];			/* Command line parameters */
	pid_t		nPid;					/* Process Id */
} proc_data_t;

class CProcUtils
{
	public:
		CProcUtils() {}
		~CProcUtils() {}

	public:
		result_t getByPid(pid_t pid, proc_data_t* pData = NULL) const;

		size_t getByNameCmd(const char* strName, const char* strSubCmd,
								proc_data_t* pData, size_t nMax = 1) const;
		size_t getByName(const char* strName, proc_data_t* pData, size_t nMax = 1) const {
			return getByNameCmd(strName, NULL, pData, nMax);
		}

		size_t killByNameCmd(const char* strName, const char* strSubCmd,
							   	hr_time_t hrTimeout = HR_30SEC) const;
		size_t killByName(const char* strName, hr_time_t hrTimeout = HR_30SEC) const {
			return killByNameCmd(strName, NULL, hrTimeout);
		}

		static void dumpProcData(const proc_data_t* pData, const char* strPref = "");

		result_t getPidFromFile(const char* strFile, pid_t* pPid) const;
		result_t sendSignal(pid_t pid, int signal) const;

	protected:
		void parseCmdLine(const char* strLine, size_t nLength, proc_data_t* pData) const;
};

#endif /* __CONTACT_PROC_UTILS_H_INCLUDED__ */
