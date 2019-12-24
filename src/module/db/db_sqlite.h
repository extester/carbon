/*
 *  Carbon/DB module
 *  Sqlite database class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 02.08.2015 12:44:03
 *      Initial revision.
 */

#ifndef __CARBON_DB_SQLITE_H_INCLUDED__
#define __CARBON_DB_SQLITE_H_INCLUDED__

#include <sqlite/sqlite3.h>

#include "carbon/lock.h"

#include "db/db_sql.h"

class CSqliteResult : public CSqlResult
{
	protected:
		sqlite3_stmt*		m_pStmt;

	public:
		CSqliteResult();
		CSqliteResult(sqlite3_stmt* pStmt);
		virtual ~CSqliteResult();

	public:
		virtual size_t getComumns() const;
		virtual SQL_VARIANT operator[](int index) const;
		virtual result_t getNextRow();

	private:
		void finalise();

};

class CDbSqlite : public CDbSql
{
	private:
		static int		m_nLibraryInitialised;
		static CMutex	m_lockInit;

		result_t		m_resultInit;
		char			m_strDatabase[256];
		sqlite3*		m_pHandle;

		CMutex			m_lockConnect;
		int				m_nConnectCount;

	public:
		CDbSqlite(const char* strDatabase);
		virtual ~CDbSqlite();

	public:
		virtual result_t connect();
		virtual void disconnect();

		virtual result_t query(const char* strQuery, CSqlResult** ppResult);
		virtual void escape(const char* strInput, char* strOutput, size_t szOutput);

	protected:
		boolean_t isConnected() const { return m_pHandle != 0; }
};

#endif /* __CARBON_DB_SQLITE_H_INCLUDED__ */
