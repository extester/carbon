/*
 *  Carbon framework
 *  Event debugging support
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 16.02.2017 11:17:10
 *      Initial revision.
 */

#include "carbon/event/event_debug.h"

std::list<event_string_table_t>	CEventStringTable::m_strTable;
CMutex							CEventStringTable::m_strTableLock;

/*
 * Register event string table
 *
 * 		nFirst			first event number
 * 		nLast			last event number
 *		strTable		string table
 *
 * Return:
 * 		ESUCCESS	successfully registered
 * 		EINVAL		invalid parameters
 */
result_t CEventStringTable::registerStringTable(event_type_t nFirst,
									event_type_t nLast, const char** strTable)
{
	CAutoLock		locker(m_strTableLock);
	result_t		nresult = ESUCCESS;
	boolean_t		bInserted = FALSE;

	event_string_table_t	est;
	std::list<event_string_table_t>::iterator	it;

	shell_assert(nFirst >= 0);
	shell_assert(nLast > nFirst);
	shell_assert(strTable);

	est.nFirst = nFirst;
	est.nLast = nLast;
	est.strTable = strTable;

	for(it=m_strTable.begin(); it!=m_strTable.end(); it++)  {
		if ( it->nFirst < nFirst && it->nFirst < nLast )  {
			m_strTable.insert(it, est);
			//log_debug(L_GEN, "[event] registered string table: %d => %d\n", nFirst, nLast);
			bInserted = TRUE;
			break;
		}

		if ( it->nLast > nFirst ) {
			continue;
		}

		log_error(L_GEN, "[event] crossong first/last events %d/%d\n", nFirst, nLast);
		nresult = EINVAL;
		break;
	}

	if ( nresult == ESUCCESS && !bInserted )  {
		/* First string table */
		//log_debug(L_GEN, "[event] registered string table: %d => %d\n", nFirst, nLast);
		m_strTable.push_back(est);
	}

	return nresult;
}

void CEventStringTable::dumpStringTable(const char* strPref)
{
	CAutoLock		locker(m_strTableLock);
	std::list<event_string_table_t>::const_iterator	it;
	int 			i;

	log_dump("*** Event String Table%s ***\n", strPref);

	for(it=m_strTable.begin(); it!=m_strTable.end(); it++)  {
		log_dump("--- events: %d => %d ---\n", it->nFirst, it->nLast);

		for(i=it->nFirst; i<=it->nLast; i++)  {
			log_dump("    %3u    %s\n", i, it->strTable[i-it->nFirst]);
		}
	}
}

void CEventStringTable::getEventName(const CEvent* pEvent, char* strBuffer,
									 size_t length)
{
	CAutoLock		locker(m_strTableLock);
	boolean_t		bFound = FALSE;
	event_type_t	type = pEvent->getType();

	std::list<event_string_table_t>::iterator	it;

	for(it=m_strTable.begin(); it!=m_strTable.end(); it++)  {
		if ( it->nFirst <= type && it->nLast >= type )  {
			copyString(strBuffer, it->strTable[type-(it->nFirst)], length);
			bFound = TRUE;
			break;
		}
	}

	if ( !bFound )  {
		_tsnprintf(strBuffer, length, "event(%u)", type);
	}
}
