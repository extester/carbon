/*
 *  Carbon/DB module
 *  SQL Database base class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 01.08.2015 22:41:40
 *      Initial revision.
 */

#include <new>

#include "shell/shell.h"

#include "carbon/memory.h"
#include "carbon/logger.h"
#include "carbon/utils.h"

#include "db/db_sql.h"


/*******************************************************************************
 * CSqlRow class
 */

CSqlRow::CSqlRow()
{
	m_arComumn.reserve(32);
}

CSqlRow::~CSqlRow()
{
	clear();
}

void CSqlRow::clear()
{
	size_t	i, count = getComumns();
	void*	p;

	for(i=0; i<count; i++)  {
		switch ( m_arComumn[i].vtype )  {
			case SQL_VT_TEXT:
				p = const_cast<char*>(m_arComumn[i].valText);
				memFree(p);
				m_arComumn[i].valText = NULL;
				break;

			case SQL_VT_BLOB:
				p = const_cast<void*>(m_arComumn[i].valBlob);
				memFree(p);
				m_arComumn[i].valBlob = NULL;
				break;

			default:
				;;
		}
	}

	m_arComumn.clear();
}

SQL_VARIANT CSqlRow::operator[](int index) const
{
	if ( (size_t)index < getComumns() )  {
		return m_arComumn[index];
	}

	SQL_VARIANT var;
	var.vtype = SQL_VT_EMPTY;
	var.vsize = 0;
	return var;
}

/*
 * Creater a permanent row object from the temporary sql result
 *
 * 		pResult		temporary sql query result
 *
 * Return: ESUCCESS, ...
 */
result_t CSqlRow::create(CSqlResult* pResult)
{
	size_t		i, count;
	result_t	nresult;

	clear();

	nresult = ESUCCESS;
	count = pResult->getComumns();

	for(i=0; i<count; i++)  {
		SQL_VARIANT		value;

		value = (*pResult)[i];
		switch ( value.vtype )  {
			case SQL_VT_EMPTY:
			case SQL_VT_NULL:
			case SQL_VT_INTEGER:
			case SQL_VT_REAL:
				break;

			case SQL_VT_TEXT:
				try {
					char*	p = (char*)memAlloc(value.vsize);
					UNALIGNED_MEMCPY(p, value.valText, value.vsize);
					value.valText = p;
				}
				catch(const std::bad_alloc& exc) {
					log_error(L_DB, "[db_sql] failed to allocate %d bytes\n", value.vsize);
					nresult = ENOMEM;
				}
				break;

			case SQL_VT_BLOB:
				try {
					void*	p = memAlloc(value.vsize);
					UNALIGNED_MEMCPY(p, value.valBlob, value.vsize);
					value.valBlob = p;
				}
				catch(const std::bad_alloc& exc) {
					log_error(L_DB, "[db_sql] failed to allocate %d bytes\n", value.vsize);
					nresult = ENOMEM;
				}
				break;

			case SQL_VT_ERROR:
			default:
				nresult = EINVAL;
				break;
		}

		if ( nresult == ESUCCESS )  {
			try {
				m_arComumn.push_back(value);
			}
			catch(const std::bad_alloc& exc) {
				log_error(L_DB, "[db_sql] failed to allocate sql row item\n");
				nresult = ENOMEM;

				if ( value.vtype == SQL_VT_TEXT || value.vtype == SQL_VT_BLOB ) {
					memFree(value.vtype == SQL_VT_TEXT ? const_cast<char*>(value.valText) :
						 const_cast<void*>(value.valBlob));
				}
			}
		}

		if ( nresult != ESUCCESS )  {
			break;
		}
	}

	return nresult;
}

/*******************************************************************************
 * CDbSql
 */

result_t CDbSql::iterateNext(CSqlResult* pResult)
{
	result_t 	nresult;

	shell_assert(pResult);

	nresult = pResult->getNextRow();
	return nresult;
}

void CDbSql::iterateEnd(CSqlResult* pResult)
{
	delete pResult;
}
