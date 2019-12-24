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
 *  Revision 1.0, 03.08.2015 00:19:17
 *      Initial revision.
 */

#ifndef __CARBON_DB_SQL_TABLE_H_INCLUDED__
#define __CARBON_DB_SQL_TABLE_H_INCLUDED__

#include "db/db_sql.h"

#define SPEC_TABLE_NAME			"@NAME@"

class CDbSqlTable
{
	protected:
		CDbSql*		m_pDb;
		char		m_strTable[256];
		size_t		m_szTable;

	public:
		CDbSqlTable(CDbSql* pDb, const char* strTable);
		virtual ~CDbSqlTable();

	public:
		size_t getSzTable() const { return m_szTable; }
		const char* getTable() const { return m_strTable; }
		void escape(const char* strInput, char* strOutput, size_t szOutput) {
			m_pDb->escape(strInput, strOutput, szOutput);
		}

		virtual result_t connect() {
			return m_pDb->connect();
		}

		virtual void disconnect() {
			m_pDb->disconnect();
		}

		virtual result_t query(const char* strQuery);
		virtual result_t getSingleValue(const char* strQuery, SQL_VARIANT* pValue);
		virtual result_t getSingleUint(const char* strQuery, uint64_t* pValue);
		virtual result_t getSingleRow(const char* strQuery, CSqlRow* pRow);
		virtual result_t getRowCount(const char* strWhereStat, size_t* pCount);
		virtual size_t getRowCount(const char* strWhereStat);

		virtual result_t iterateStart(const char* strQuery, CSqlResult** ppResult) {
			return doQuery(strQuery, ppResult);
		}
		virtual result_t iterateNext(CSqlResult* pResult) {
			return m_pDb->iterateNext(pResult);
		}
		virtual void iterateEnd(CSqlResult* pResult) {
			return m_pDb->iterateEnd(pResult);
		}

	protected:
		virtual result_t lockExclusive();
		virtual result_t unlock();
		virtual result_t recordInsert(const char* strQuery, uint32_t* pId);
		virtual result_t recordDelete(const char* strWhere);
		virtual result_t recordDelete(uint32_t id);

	private:
		result_t doQuery(const char* strQuery, CSqlResult** ppResult);
};

#endif /* __CARBON_DB_SQL_TABLE_H_INCLUDED__ */
