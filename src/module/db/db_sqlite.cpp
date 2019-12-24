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
 *  Revision 1.0, 02.08.2015 12:54:01
 *      Initial revision.
 */

#include <new>

#include "shell/shell.h"
#include "carbon/logger.h"

#include "db/db_sqlite.h"

static result_t sqlite2nresult(int retVal)
{
	result_t	nresult;

	switch (retVal)  {
		case SQLITE_OK:			nresult = ESUCCESS; break;
		case SQLITE_ABORT:		nresult = ECANCELED; break;
		case SQLITE_CANTOPEN:	nresult = ENOENT; break;
		case SQLITE_ERROR:		nresult = EIO; break;
		case SQLITE_INTERRUPT:	nresult = EINTR; break;
		case SQLITE_IOERR:		nresult = EIO; break;
		case SQLITE_NOMEM:		nresult = ENOMEM; break;
		case SQLITE_NOTFOUND:	nresult = ENOENT; break;
		case SQLITE_PERM:		nresult = EACCES; break;
		case SQLITE_RANGE:		nresult = ERANGE; break;
		case SQLITE_READONLY:	nresult = EROFS; break;
		default: nresult = EIO; break;
	}

	return nresult;
}

/*******************************************************************************
 * CSqliteResult class
 */

CSqliteResult::CSqliteResult() :
	CSqlResult(),
	m_pStmt(0)
{
}

CSqliteResult::CSqliteResult(sqlite3_stmt* pStmt) :
	CSqlResult(),
	m_pStmt(pStmt)
{
}

CSqliteResult::~CSqliteResult()
{
	finalise();
}


void CSqliteResult::finalise()
{
	if ( m_pStmt != 0 )  {
		sqlite3_finalize(m_pStmt);
		m_pStmt = 0;
	}
}


size_t CSqliteResult::getComumns() const
{
	size_t	nColumns = 0;

	if ( m_pStmt != 0 )  {
		nColumns = (size_t)sqlite3_column_count(m_pStmt);
	}

	return nColumns;
}

SQL_VARIANT CSqliteResult::operator[](int index) const
{
	SQL_VARIANT var;

	if ( m_pStmt != 0 && (size_t)index < getComumns() ) {
		int column_type;

		column_type = sqlite3_column_type(m_pStmt, index);
		switch (column_type) {
			case SQLITE_INTEGER:
				var.vtype = SQL_VT_INTEGER;
				var.vsize = sizeof(var.valInt);
				var.valInt = (uint64_t)sqlite3_column_int64(m_pStmt, index);
				break;

			case SQLITE_FLOAT:
				var.vtype = SQL_VT_REAL;
				var.vsize = sizeof(var.valDouble);
				var.valDouble = sqlite3_column_double(m_pStmt, index);
				break;

			case SQLITE_TEXT:
				var.vtype = SQL_VT_TEXT;
				var.vsize = (uint32_t)sqlite3_column_bytes(m_pStmt, index);
				var.valText = (const char*)sqlite3_column_text(m_pStmt, index);
				break;

			case SQLITE_BLOB:
				var.vtype = SQL_VT_BLOB;
				var.vsize = (uint32_t)sqlite3_column_bytes(m_pStmt, index);
				var.valBlob = sqlite3_column_blob(m_pStmt, index);
				break;

			case SQLITE_NULL:
				var.vtype = SQL_VT_NULL;
				var.vsize = 0;
				break;

			default:
				log_debug(L_DB, "[sqlite] unexpected column type %d\n", column_type);
				var.vtype = SQL_VT_ERROR;
				var.vsize = 0;
				break;
		}
	}
	else {
		var.vtype = SQL_VT_EMPTY;
		var.vsize = 0;
	}

	return var;
}

result_t CSqliteResult::getNextRow()
{
	result_t	nresult = ENODATA;

	if ( m_pStmt )  {
		int 	retVal;

		retVal = sqlite3_step(m_pStmt);
		switch (retVal)  {
			case SQLITE_ROW:		/* Got a row */
				nresult = ESUCCESS;
				break;

			case SQLITE_DONE:
				nresult = ENODATA;
				break;

			default:
				nresult = sqlite2nresult(retVal);
				break;
		}
	}

	return nresult;
}


/*******************************************************************************
 * CDbSqlite class
 */

int CDbSqlite::m_nLibraryInitialised = 0;
CMutex CDbSqlite::m_lockInit;

CDbSqlite::CDbSqlite(const char* strDatabase) :
	CDbSql("Sqlite"),
	m_pHandle(NULL),
	m_nConnectCount(0)
{
	copyString(m_strDatabase, strDatabase, sizeof(m_strDatabase));

	CAutoLock	locker(m_lockInit);

	m_resultInit = ESUCCESS;
	if ( m_nLibraryInitialised < 1 )  {
		int		retVal;

		retVal = sqlite3_initialize();
		if ( retVal == SQLITE_OK ) {
			m_nLibraryInitialised++;
		}
		else {
			log_debug(L_DB, "[sqlite] low level library initialisation failed, sqlite result: %d\n", retVal);
			m_resultInit = sqlite2nresult(retVal);
		}
	}
}

CDbSqlite::~CDbSqlite()
{
	CAutoLock	locker(m_lockInit);

	shell_assert(m_pHandle == NULL);
	shell_assert(m_nConnectCount == 0);

	if ( m_resultInit == ESUCCESS )  {
		m_nLibraryInitialised--;
		if ( m_nLibraryInitialised < 1 ) {
			int		retVal;

			retVal = sqlite3_shutdown();
			if ( retVal != SQLITE_OK ) {
				log_debug(L_DB, "[sqlite] low level library shutdown failed, sqlite result: %d\n", retVal);
			}
		}
	}
}

result_t CDbSqlite::connect()
{
	CAutoLock	locker(m_lockInit);
	result_t	nresult = ESUCCESS;

	if ( m_resultInit != ESUCCESS )  {
		return m_resultInit;
	}

	shell_assert(m_nConnectCount >= 0);
	if ( m_nConnectCount < 1 )  {
		int		retVal;

		shell_assert(m_pHandle == NULL);
		retVal = sqlite3_open(m_strDatabase, &m_pHandle);
		if ( retVal != SQLITE_OK ) {
			const char*		strErr;

			strErr = sqlite3_errstr(retVal);
			log_debug(L_DB, "[sqlite] failed to open db %s, sqlite result: %s(%d)\n",
					  m_strDatabase, strErr, retVal);

			sqlite3_close(m_pHandle);
			m_pHandle = NULL;
			nresult = sqlite2nresult(retVal);
		}
	}

	if ( nresult == ESUCCESS )  {
		m_nConnectCount++;
	}

	return nresult;
}

void CDbSqlite::disconnect()
{
	CAutoLock	locker(m_lockInit);

	shell_assert(m_pHandle != NULL);
	shell_assert(m_nConnectCount > 0);
	if ( m_nConnectCount < 2 )  {
		int		retVal;

		retVal = sqlite3_close(m_pHandle);
		if ( retVal != SQLITE_OK )  {
			const char*		strErr;

			strErr = sqlite3_errstr(retVal);
			log_debug(L_GEN, "[sqlite] failed to close db %s, sqlite result: %s(%d)\n",
					  m_strDatabase, strErr, retVal);
		}
		m_pHandle = NULL;
	}

	m_nConnectCount--;
}

result_t CDbSqlite::query(const char* strQuery, CSqlResult** ppResult)
{
	result_t	nresult = EBADF;

	shell_assert(strQuery);
	shell_assert(ppResult);

	*ppResult = 0;

	if ( isConnected() )  {
		sqlite3_stmt*	pStmt = 0;
		int 			retVal;

		log_debug(L_DB_SQL_TRACE, "[sqlite] SQL: '%s'\n", strQuery);

		retVal = sqlite3_prepare_v2(m_pHandle, strQuery, -1, &pStmt, NULL);
		if ( retVal == SQLITE_OK ) {
			CSqliteResult*	pResult;

			retVal = sqlite3_step(pStmt);
			//log_debug(L_DB_SQL_TRACE, "[sqlite] SQL RESULT: '%d'\n", retVal);

			switch (retVal)  {
				case SQLITE_ROW:		/* Got a row */
					try {
						pResult = new CSqliteResult(pStmt);
						*ppResult = pResult;
						nresult = ESUCCESS;
					}
					catch(const std::bad_alloc& exc)  {
						log_error(L_DB, "[sqlite] memory allocation failed\n");
						nresult = ENOMEM;
					}
					break;

				case SQLITE_DONE:
					try {
						pResult = new CSqliteResult();
						*ppResult = pResult;
						nresult = ENODATA;
					}
					catch(const std::bad_alloc& exc)  {
						log_error(L_DB, "[sqlite] memory allocation failed\n");
						nresult = ENOMEM;
					}
					break;

				default:
					nresult = sqlite2nresult(retVal);
					break;
			}

			if ( nresult != ESUCCESS )  {
				sqlite3_finalize(pStmt);
				if ( nresult == ENODATA )  {
					nresult = ESUCCESS;
				}
			}
		}
		else {
			const char*		strErr;

			strErr = sqlite3_errmsg(m_pHandle);
			log_debug(L_DB, "[sqlite] query '%s' failed, result: %s(%u)\n", strQuery, strErr, retVal);
			nresult = sqlite2nresult(retVal);
		}
	}

	return nresult;
}


void CDbSqlite::escape(const char* strInput, char* strOutput, size_t szOutput)
{
	const char*	s = strInput;
	ssize_t		length = 0, maxLength = (ssize_t)szOutput-1;

	while( *s && length < maxLength )  {
		strOutput[length] = *s;
		length++;
		if ( length == maxLength )  {
			break;
		}
		if ( *s == '\'' )  {
			strOutput[length] = *s;
			length++;
		}
		s++;
	}

	strOutput[length] = '\0';
}
