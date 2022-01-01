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
 *  Revision 1.0, 03.11.2017 10:54:41
 *      Initial revision.
 */

#include <dirent.h>
#include <signal.h>

#include "shell/logger.h"
#include "shell/tstring.h"
#include "shell/file.h"

#include "carbon/cstring.h"
#include "contact/proc_utils.h"


/*******************************************************************************
 * CProcUtils class
 */

/*
 * Parse cmd line of the found process and fill proc data
 *
 * 		strLine			process cmd line
 * 		nLength			cmd line length, bytes
 * 		pData			proc data to fill
 */
void CProcUtils::parseCmdLine(const char* strLine, size_t nLength, proc_data_t* pData) const
{
	*pData->strName = '\0';
	*pData->strParams = '\0';

	if ( nLength > 0 ) {
		const char 		*s, *p;
		char*			pp;
		size_t			l;

		s = strLine;
		MSKIP_CHARS(s, nLength, " \n\r");

		/* Get executable file short name */
		l = nLength;
		if ( _tmemchr(s, '\0', l) != 0 )  {
			l = _tstrlen(s);
		}

		p = (const char*)_tmemrchr(s, PATH_SEPARATOR, l);
		if ( !p ) {
			p = s;
		}
		else {
			p++;
			l -= A(p)-A(s);
		}

		copySubstring(pData->strName, p, l, sizeof(pData->strName));
		s = p+l;
		nLength -= A(s) - A(strLine);

		if ( nLength > 0 && *s == '\0' )  {
			s++; nLength--;
		}

		/* Get and join parameters */
		copySubstring(pData->strParams, s, nLength, sizeof(pData->strParams));
		l = sh_min(nLength, sizeof(pData->strParams)-1);

		while ( (pp=(char*)_tmemchr(pData->strParams, '\0', l)) != 0 )  {
			*pp = ' ';
		}
	}
}

/*
 * Get process parameters by PID
 *
 * 		pid			process Id to find
 * 		pData		process data [out], may be nullptr
 *
 * Return:
 * 		ESUCCESS	process found and parameters are filled
 * 		ENOENT		process not found
 * 		...			read error
 */
result_t CProcUtils::getByPid(pid_t pid, proc_data_t* pData) const
{
	char		strBuf[512];
	CFile		file;
	size_t		size;
	result_t	nresult;

	_tsnprintf(strBuf, sizeof(strBuf), "/proc/%u/cmdline", pid);

	nresult = file.open(strBuf, CFile::fileRead);
	if ( nresult == ESUCCESS ) {
		size = sizeof(strBuf)-1;
		nresult = file.read(strBuf, &size);
		if ( nresult == ESUCCESS && pData != NULL ) {
			pData->nPid = pid;
			parseCmdLine(strBuf, size, pData);
		}

		file.close();
	}

	return nresult;
}

/*
 * Get process parameters by name/cmd
 *
 * 		strName			process name to find (filename only, no path)
 * 		strSubCmd		process parameters substring (may be nullptr)
 * 		pData			process parameters array [out]
 * 		nMax			maximum processes in output array
 * 		pCount			found processes (may be nullptr) [out]
 *
 * Return:
 * 		ESUCCESS		0 or more processes found
 * 		...				error while parsing, pCount contains count of the
 * 						successfully found processes
 */
result_t CProcUtils::getByNameCmd(const char* strName, const char* strSubCmd,
					  	proc_data_t* pData, size_t nMax, size_t* pCount) const
{
	char				strBuf[512];
	DIR*				dir;
	char*				endp;
	long				pid;
	struct dirent*		dirent;
	size_t				count = 0;

	CFile				file;
	size_t				size;
	result_t			nresult = ESUCCESS, nr;

	shell_assert(pData != nullptr);

	dir = opendir("/proc");
	if ( !dir )  {
		if ( pCount) *pCount = 0;
		return errno;
	}

	while ( (dirent=readdir(dir)) != nullptr )  {
		pid = _tstrtol(dirent->d_name, &endp, 10);
		if ( *endp != '\0' )  {
			continue;
		}

		if ( count >= nMax )  {
			break;
		}

		_tsnprintf(strBuf, sizeof(strBuf), "/proc/%lu/cmdline", pid);

		nr = file.open(strBuf, CFile::fileRead);
		if ( nr == ESUCCESS ) {
			size = sizeof(strBuf)-1;
			nr = file.read(strBuf, &size);
			if ( nr == ESUCCESS ) {
				proc_data_t		d;

				parseCmdLine(strBuf, size, &d);
				if ( _tstrlen(d.strName) > 0 && _tstrcmp(d.strName, strName) == 0 ) {
					if ( strSubCmd == nullptr || _tstrstr(d.strParams, strSubCmd) != nullptr ) {
						pData[count].nPid = (pid_t)pid;
						copyString(pData[count].strName, d.strName, sizeof(pData[count].strName));
						copyString(pData[count].strParams, d.strParams, sizeof(pData[count].strParams));
						count++;
					}
				}
			}

			file.close();
		}

		nresult_join(nresult, nr);
	}

	closedir(dir);

	if ( pCount ) *pCount = count;
	return nresult;
}

/*
 * Kill a specified processes
 *
 * 		strName			process name (filename without path)
 * 		strSubCmd		part of the process command line (may be nullptr)
 * 		hrTimeout		maximum timeout for the killing all processes
 * 		pCount			killed processes (may be nullptr) [out]
 *
 * Return:
 * 		ESUCCESS		0 or more processes were killed
 * 		...				error while killing, pCount contains count of the
 * 						successfully killed processes
 */
result_t CProcUtils::killByNameCmd(const char* strName, const char* strSubCmd,
								 	hr_time_t hrTimeout, size_t* pCount) const
{
	proc_data_t		pdata;
	size_t			count = 0, n;
	hr_time_t		hrStart;
	int				i;
	result_t		nresult = ESUCCESS;
	const int		nIterTerm = (int)(hrTimeout/HR_200MSEC), nIterKill = nIterTerm/2;

	shell_assert(strName);
	shell_assert(hrTimeout >= HR_200MSEC);

	hrStart = hr_time_now();

	while ( hr_time_get_elapsed(hrStart) < hrTimeout ) {
		nresult = getByNameCmd(strName, strSubCmd, &pdata, 1, &n);
		if ( nresult != ESUCCESS || n == 0 )  {
			break;
		}

		shell_assert(pdata.nPid != 0);

		for(i=0; i<nIterTerm; i++)  {
			kill(pdata.nPid, SIGTERM);

			if ( getByPid(pdata.nPid) == ESUCCESS )  {
				hr_sleep(HR_200MSEC);
			}
			else {
				count++;
				break /*for*/;
			}
		}

		if ( getByPid(pdata.nPid) == ESUCCESS ) {
			log_debug(L_GEN, "[proc_utils] terminate (term) '%s' pid=%u failed\n",
										strName, pdata.nPid);

			for(i=0; i<nIterKill; i++)  {
				kill(pdata.nPid, SIGKILL);

				if ( getByPid(pdata.nPid) == ESUCCESS )  {
					hr_sleep(HR_200MSEC);
				}
				else {
					count++;
					break /*for*/;
				}
			}
		}

		if ( getByPid(pdata.nPid) == ESUCCESS ) {
			log_error(L_GEN, "[proc_utils] kill (kill) '%s' pid=%u failed\n",
					  				strName, pdata.nPid);
			nresult = EBUSY;
			break /*while*/;
		}
	}

	if ( pCount ) *pCount = count;
	return nresult;
}

/*
 * Read PID from file
 *
 * 		strFile			full filename containing PID
 * 		pPid			output PID
 *
 * Return: ESUCCESS, ...
 */
result_t CProcUtils::getPidFromFile(const char* strFile, pid_t* pPid) const
{
	CFile			file;
	CString			s;
	unsigned int	pid;
	char			buffer[64];
	size_t			length;
	result_t 		nresult;

	nresult = file.open(strFile, CFile::fileRead);
	if ( nresult == ESUCCESS )  {
		length = sizeof(buffer)-1;
		nresult = file.read(buffer, &length);
		if ( nresult == ESUCCESS ) {
			buffer[length] = '\0';
			rTrim(buffer, " \t\r\n");

			s = buffer;
			nresult = s.getNumber(pid, 10);
			if ( nresult == ESUCCESS )  {
				*pPid = (pid_t)pid;
			}
		}

		file.close();
	}

	return nresult;
}

/*
 * Send a signal to a process
 *
 * 		pid			process ID to send signal
 * 		signal		signal to send, SIGxxx
 *
 * Return: ESUCCESS, ...
 */
result_t CProcUtils::sendSignal(pid_t pid, int signal) const
{
	int			retVal;
	result_t	nresult = ESUCCESS;

	retVal = ::kill(pid, signal);
	if ( retVal < 0 )  {
		nresult = errno;
	}

	return nresult;
}

/*******************************************************************************
 * Debugging support
 */

/*
 * Dump proc data
 *
 * 		pData			data to dump
 * 		strPref			dump line prefix (optional)
 */
void CProcUtils::dumpProcData(const proc_data_t* pData, const char* strPref)
{
	shell_assert(pData);

	log_dump("*** %sProcData: pid=%lu\n", strPref, pData->nPid);
	log_dump("      Short executable: '%s'\n", pData->strName);
	log_dump("      Parameters:       '%s'\n", pData->strParams);
}
