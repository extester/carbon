/*
 *  Carbon/DB module
 *  SQL Table base class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.08.2015 00:31:17
 *      Initial revision.
 */

#include "carbon/logger.h"
#include "carbon/memory.h"

#include "db/db_sql_table.h"


CDbSqlTable::CDbSqlTable(CDbSql* pDb, const char* strTable) :
	m_pDb(pDb)
{
	shell_assert(pDb);
	shell_assert(strTable);
	copyString(m_strTable, strTable, sizeof(m_strTable));
	m_szTable = _tstrlen(m_strTable);
}

CDbSqlTable::~CDbSqlTable()
{
}


result_t CDbSqlTable::doQuery(const char* strQuery, CSqlResult** ppResult)
{
	char*			strTmpQuery;
	const char*		p;
	size_t			lq, l1, l2;
	static size_t	lspec = 0;

	if ( (p=_tstrstr(strQuery, SPEC_TABLE_NAME)) == NULL )  {
		return m_pDb->query(strQuery, ppResult);
	}

	if ( !lspec )  {
		lspec = _tstrlen(SPEC_TABLE_NAME);
	}

	lq = _tstrlen(strQuery);
	strTmpQuery = (char*)memAlloca(lq+lspec+1);

	l1 = A(p) - A(strQuery);

	UNALIGNED_MEMCPY(strTmpQuery, strQuery, l1);
	UNALIGNED_MEMCPY(&strTmpQuery[l1], m_strTable, m_szTable);
	l2 = lq-l1-lspec;
	UNALIGNED_MEMCPY(&strTmpQuery[l1+m_szTable], p+lspec, l2);
	strTmpQuery[l1+m_szTable+l2] = '\0';

	return m_pDb->query(strTmpQuery, ppResult);
}

result_t CDbSqlTable::query(const char* strQuery)
{
	CSqlResult*		pSqlResult;
	result_t		nresult;

	nresult = doQuery(strQuery, &pSqlResult);
	delete pSqlResult;

	return nresult;
}

result_t CDbSqlTable::getSingleValue(const char* strQuery, SQL_VARIANT* pValue)
{
	CSqlResult*		pSqlResult;
	result_t		nresult;

	shell_assert(strQuery);

	nresult = doQuery(strQuery, &pSqlResult);
	if ( nresult == ESUCCESS )  {
		if ( pValue ) {
			*pValue = (*pSqlResult)[0];
		}
		delete pSqlResult;
	}

	return nresult;
}

result_t CDbSqlTable::getSingleUint(const char* strQuery, uint64_t* pValue)
{
	SQL_VARIANT		var;
	result_t		nresult;

	nresult = getSingleValue(strQuery, &var);
	if ( nresult == ESUCCESS )  {
		if ( var.vtype == SQL_VT_INTEGER )  {
			*pValue = var.valInt;
		}
		else {
			log_debug(L_DB, "[sql_tab] invalid sql query for UINT, type %d\n", var.vtype);
			nresult = EINVAL;
		}
	}

	return nresult;
}

result_t CDbSqlTable::getSingleRow(const char* strQuery, CSqlRow* pRow)
{
	CSqlResult*		pSqlResult;
	result_t		nresult;

	shell_assert(strQuery);
	shell_assert(pRow);

	nresult = doQuery(strQuery, &pSqlResult);
	if ( nresult == ESUCCESS )  {
		nresult = pRow->create(pSqlResult);
		delete pSqlResult;
	}

	return nresult;
}

result_t CDbSqlTable::getRowCount(const char* strWhereStat, size_t* pCount)
{
	char*			strTmp;
	uint64_t		count;
	size_t			l;
	result_t		nresult;

	shell_assert(strWhereStat);
	shell_assert(pCount);

	l = (strWhereStat ? _tstrlen(strWhereStat) : 2) + m_szTable + 128;
	strTmp = (char*)memAlloca(l);

	if ( strWhereStat )  {
		_tsnprintf(strTmp, l, "SELECT COUNT(*) FROM %s WHERE %s", m_strTable, strWhereStat);
	}
	else  {
		_tsnprintf(strTmp, l, "SELECT COUNT(*) FROM %s", m_strTable);
	}

	nresult = getSingleUint(strTmp, &count);
	if ( nresult == ESUCCESS )  {
		*pCount = (size_t)count;
	}

	return nresult;
}

size_t CDbSqlTable::getRowCount(const char* strWhereStat)
{
	size_t		count;
	result_t	nresult;

	nresult = getRowCount(strWhereStat, &count);
	return nresult == ESUCCESS ? count : 0;
}

result_t CDbSqlTable::lockExclusive()
{
	result_t	nresult;

	nresult = query("BEGIN EXCLUSIVE TRANSACTION");
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[sql_tab] failed to start exclusive transaction, result: %d\n", nresult);
	}

	return nresult;
}

result_t CDbSqlTable::unlock()
{
	result_t	nresult;

	nresult = query("COMMIT");
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[sql_tab] failed to commit transaction, result: %d\n", nresult);
	}

	return nresult;
}

/*
 * Insert a new record to the table
 *
 * 		strQuery		inserting query
 * 		pId				inserted record Id [out]
 *
 * Return: ESUCCESS, ...
 */
result_t CDbSqlTable::recordInsert(const char* strQuery, uint32_t* pId)
{
	char*			strQuery1;
	SQL_VARIANT		result;
	result_t		nresult;

	strQuery1 = (char*)memAlloca(m_szTable+128);

	lockExclusive();
	nresult = query(strQuery);
	if ( nresult == ESUCCESS )  {
		_tsnprintf(strQuery1, m_szTable+128, "SELECT MAX(id) FROM %s", m_strTable);
		nresult = getSingleValue(strQuery1, &result);
		if ( nresult == ESUCCESS )  {
			if ( result.vtype == SQL_VT_INTEGER )  {
				*pId = (uint32_t)result.valInt;
			}
			else {
				log_debug(L_DB, "[sql_tab] wrong result type %d, possible invalid query %s\n",
						  result.vtype, strQuery);
				nresult = EINVAL;
			}
		}
	}
	unlock();

	return nresult;
}

/*
 * Delete record(s) based on WHERE statement
 *
 * 		strWhere		'where' statement
 *
 * Return: ESUCCESS, ...
 */
result_t CDbSqlTable::recordDelete(const char* strWhere)
{
	char*		strQuery;
	size_t		lw;
	result_t	nresult;

	shell_assert(strWhere);

	lw = _tstrlen(strWhere);
	strQuery = (char*)memAlloca(lw+m_szTable+64);

	_tsnprintf(strQuery, lw+m_szTable+64, "DELETE FROM %s WHERE %s\n", strWhere, m_strTable);

	nresult = query(strQuery);
	return nresult;
}

/*
 * Delete record of ID
 *
 * 		id			record id to delete
 *
 * Return: ESUCCESS, ...
 */
result_t CDbSqlTable::recordDelete(uint32_t id)
{
	char*		strQuery;
	result_t	nresult;

	shell_assert(id != 0);

	strQuery = (char*)memAlloca(m_szTable+64);
	_tsnprintf(strQuery, m_szTable+64, "DELETE FROM %s WHERE id = %d", m_strTable, id);

	nresult = query(strQuery);
	return nresult;
}

