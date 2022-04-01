/*
 *  Telemetrica TeleGeo backend server
 *  MySQL Database
 *
 *  Copyright (c) 2020 Telemetrica. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.08.2020 12:39:33
 *      Initial revision.
 */
/*
 * Initialisation:
 *
 * 	int main(int argc, char* argv[])
 * 	{
 * 		mysql_library_init();	// is not thread-safe, calls mysql_thread_init()
 *
 * 		MYSQL* mysql = mysql_init(NULL);	// calls mysql_thread_init()
 *
 * 		start_thread(thread_func, data);
 *
 * 		mysql_library_end();
 * 	}
 *
 * 	void thread_func(void* data)
 * 	{
 * 		mysql_thread_init();
 * 		...
 * 		mysql_thread_end();
 * 	}
 *
 */
/*
 * API:
 *
 * 		result_t query(const char* strQuery);
 *
 *		result_t queryValue(const char* strQuery, CString* pString);
 *		result_t queryValue(const char* strQuery, uint64_t* pValue);
 *		result_t queryValue(const char* strQuery, int64_t* pValue);
 *		result_t queryValue(const char* strQuery, uint32_t* pValue);
 *		result_t queryValue(const char* strQuery, int32_t* pValue);
 *
 *		result_t queryRow(const char* strQuery, str_vector_t* pVector);
 *
 *		try {
 *			CMySqlResult	res;
 *			CMySqlRow		row;
 *			size_t			nFields;
 *
 *			db.iterate("SELECT 11* FROM test", &res);
 *			nFields = res.getFields();
 *
 *			while ( res.getRow(&row) )  {
 *				for(size_t i=0; i<count; i++)  {
 *					log_dump("%s\n", row[i]);
 *				}
 *			}
 *		}
 *		catch(std::sql_exception& ex)  {
 *			log_error(L_SQL, "[mysql] database iterate failed, result %d\n", ex.getResult());
 *		}
 */

#include <stdexcept>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <mysql/mysqld_error.h>

#include "carbon/memory.h"
#include "carbon/logger.h"

#include "db/db_mysql.h"
#include "db/db_mysql_result.h"

#define MYSQL_AUTORECONNECT_DEFAULT			3
#define MYSQL_CHARSET_DEFAULT				"utf8"
#define MYSQL_COLLATION_DEFAULT				"utf8_general_ci"

boolean_t CDbMySql::m_bDbMySqlLibraryInitialised = FALSE;
__thread boolean_t CDbMySql::m_bDbMysqlThreadInitialised = FALSE;

/*
 * [Static]
 *
 * Function initialises the MySQL client library before call any other MySQL function
 *
 * Return: ESUCCESS, ...
 */
result_t CDbMySql::initLibrary()
{
	int			nr;
	result_t	nresult = ESUCCESS;

	shell_assert_ex(!m_bDbMySqlLibraryInitialised, "MySql library already initialised");

	nr = mysql_library_init(0, nullptr, nullptr);
	if ( nr == 0 ) {
		m_bDbMySqlLibraryInitialised = TRUE;
	}
	else {
		log_error(L_SQL, "[mysql] could not initialise MySQL client library\n");
		nresult = EFAULT;
	}

	return nresult;
}

/*
 * [Static]
 *
 * This function finalizes the MySQL library and frees any allocated resources
 *
 */
void CDbMySql::terminateLibrary()
{
	if ( m_bDbMySqlLibraryInitialised ) {
		mysql_library_end();
		m_bDbMySqlLibraryInitialised = FALSE;
	}
}

/*
 * [Static Per-Thread]
 *
 * Initialise per-thread mysql library data
 *
 * Return: ESUCCESS, EIO
 */
result_t CDbMySql::initThread()
{
	result_t	nresult = ESUCCESS;

	if ( !m_bDbMysqlThreadInitialised ) {
		if ( ::mysql_thread_init() == 0 ) {
			m_bDbMysqlThreadInitialised = TRUE;
			log_trace(L_SQL, "[mysql] client thread initialised\n");
		}
		else {
			log_error(L_SQL, "[mysql] failure thread data initialisation\n");
			nresult = EIO;
		}
	}

	return nresult;
}

/*
 * [Static Per-Thread]
 *
 * Deallocating per-thread resources
 */
void CDbMySql::terminateThread()
{
	if ( m_bDbMysqlThreadInitialised ) {
		::mysql_thread_end();
		m_bDbMysqlThreadInitialised = FALSE;
	}
}

const char* CDbMySql::getClientVersion()
{
	return ::mysql_get_client_info();
}

/*
 * Convert MySQL result code to Carbon result code
 *
 * 		nErr		MySQL error code
 *
 * Return: carbon result code
 */
result_t errMySql2Nr(unsigned int nErr)
{
	result_t	nresult;

	switch ( nErr ) {
		case ER_DUP_ENTRY:					nresult = EEXIST; break;
		case CR_OUT_OF_MEMORY:				nresult = ENOMEM; break;
		case ER_NO_SUCH_TABLE:				nresult = ENOENT; break;
		default:							nresult = EIO; break;
	}

	return nresult;
}

/*******************************************************************************
 * CDbMySql class
 */

/*
 * Mysql database object constructor
 *
 * 		strDb			database name to select (may not be nullptr, use "")
 */
CDbMySql::CDbMySql(const CNetAddr& server, const char* strUser, const char* strPass,
		 const char* strCharset, const char* strCollation,
		 const char* strDb, const char* strName) :
	CDbSql(strName),
	m_server(server),
	m_strUser(strUser),
	m_strPass(strPass),
	m_strCharset(strCharset),
	m_strCollation(strCollation),
	m_strDb(strDb),
	m_pHandle(nullptr),
	m_nAutoReconnect(MYSQL_AUTORECONNECT_DEFAULT),
	m_bMultiStmt(false),
	m_nResultCount(ZERO_ATOMIC)
{
}

CDbMySql::CDbMySql(const CNetAddr& server, const char* strUser, const char* strPass,
				   const char* strDb, const char* strName) :
	CDbSql(strName),
	m_server(server),
	m_strUser(strUser),
	m_strPass(strPass),
	m_strCharset(MYSQL_CHARSET_DEFAULT),
	m_strCollation(MYSQL_COLLATION_DEFAULT),
	m_strDb(strDb),
	m_pHandle(nullptr),
	m_nAutoReconnect(MYSQL_AUTORECONNECT_DEFAULT),
	m_bMultiStmt(false),
	m_nResultCount(ZERO_ATOMIC)
{
}

CDbMySql::~CDbMySql()
{
	/*
	 * Check for lost query results
	 */
	if ( sh_atomic_get(&m_nResultCount) != 0 )  {
		log_error(L_SQL, "[sql] **********************************\n");
		log_error(L_SQL, "[sql] lost %ld query result(s)\n", sh_atomic_get(&m_nResultCount));
		log_error(L_SQL, "[sql] **********************************\n");
	}
}

/*
 * Set autoreconnect option
 *
 * 		nAutoReconnect			> 0 - use nAutoReconnect attempts
 * 								0 - do not automatically reconnect
 */
void CDbMySql::setAutoReconnect(int nAutoReconnect)
{
	m_nAutoReconnect = nAutoReconnect;
}

/*
 * Connect to MySql database, selects database, charset and collation
 *
 * Return:
 * 		ESUCCESS		connected
 * 		ENOMEM			out of memory error
 * 		EIO				connection failed
 *
 * Note: Mutual lock must be held
 */
result_t CDbMySql::doConnect()
{
	MYSQL			*pHandle, *pH;
	CString			strQuery;
	const char*		strErr;
	unsigned int	nErr, timeout;
	unsigned long	clientFlag;
	int				nr;
	result_t		nresult;

	shell_assert(!m_pHandle);

	shell_assert_ex(m_bDbMySqlLibraryInitialised, "MySql library is not "
					  "initialised, call CDbMySql::initLibrary()\n");

	log_trace(L_SQL, "[mysql] connecting to %s@%s, database '%s'\n",
			  				m_server.cs(), m_strUser.cs(), m_strDb.cs());

	pHandle = mysql_init(nullptr);
	if ( !pHandle )  {
		log_error(L_SQL, "[mysql] failed to allocate handle\n");
		return ENOMEM;
	}

	/*
	 * Setup connect timeout
	 */
	if ( m_hrConnectTimeout != HR_0 )  {
		timeout = HR_TIME_TO_SECONDS(m_hrConnectTimeout);

		if ( timeout < 1 )  {
			log_warning(L_SQL, "[mysql] connect timeout < 1s, use 1s timeout\n");
			timeout = 1;
		}

		nr = mysql_options(pHandle, MYSQL_OPT_CONNECT_TIMEOUT, (void*)&timeout);
		if ( nr != 0 )  {
			strErr = ::mysql_error(pHandle);
			nErr = ::mysql_errno(pHandle);

			nresult = errMySql2Nr(nErr);

			log_warning(L_SQL, "[mysql] connect timeout is not set: result %d, mysql error %d (%s)\n",
					  					nresult, nErr, strErr);
		}
	}

	/*
	 * Setup send query timeout
	 */
	if ( m_hrSendTimeout != HR_0 )  {
		timeout = HR_TIME_TO_SECONDS(m_hrSendTimeout);

		if ( timeout < 1 )  {
			log_warning(L_SQL, "[mysql] send timeout < 1s, use 1s timeout\n");
			timeout = 1;
		}

		nr = mysql_options(pHandle, MYSQL_OPT_WRITE_TIMEOUT, (void*)&timeout);
		if ( nr != 0 )  {
			strErr = ::mysql_error(pHandle);
			nErr = ::mysql_errno(pHandle);

			nresult = errMySql2Nr(nErr);

			log_warning(L_SQL, "[mysql] send timeout is not set: result %d, mysql error %d (%s)\n",
						nresult, nErr, strErr);
		}
	}

	/*
	 * Setup receive result timeout
	 */
	if ( m_hrRecvTimeout != HR_0 )  {
		timeout = HR_TIME_TO_SECONDS(m_hrRecvTimeout);

		if ( timeout < 1 )  {
			log_warning(L_SQL, "[mysql] receive timeout < 1s, use 1s timeout\n");
			timeout = 1;
		}

		nr = mysql_optionsv(pHandle, MYSQL_OPT_READ_TIMEOUT, (void*)&timeout);
		if ( nr != 0 )  {
			strErr = ::mysql_error(pHandle);
			nErr = ::mysql_errno(pHandle);

			nresult = errMySql2Nr(nErr);

			log_warning(L_SQL, "[mysql] send timeout is not set: result %d, mysql error %d (%s)\n",
						nresult, nErr, strErr);
		}
	}

	clientFlag = m_bMultiStmt ? CLIENT_MULTI_STATEMENTS : 0;

	pH = ::mysql_real_connect(pHandle, m_server.getHost(),
				m_strUser.cs(), m_strPass.cs(), m_strDb.isEmpty() ? nullptr : m_strDb.cs(),
				m_server.getPort(), nullptr,clientFlag);

	if ( !pH )  {
		strErr = ::mysql_error(pHandle);
		nErr = ::mysql_errno(pHandle);

		nresult = errMySql2Nr(nErr);

		log_error(L_SQL, "[mysql] can't connect to %s, result %d, mysql error %d (%s)\n",
						m_server.cs(), nresult, nErr, strErr);
		::mysql_close(pHandle);

		return nresult;
	}

	nr = mysql_set_character_set(pHandle, m_strCharset);
	if ( nr != 0 )  {
		strErr = ::mysql_error(pHandle);
		nErr = ::mysql_errno(pHandle);

		nresult = errMySql2Nr(nErr);

		log_error(L_SQL, "[mysql] can't set charset '%s', result %d, mysql error %d (%s)\n",
				  		m_strCharset.cs(), nresult, nErr, strErr);
		::mysql_close(pHandle);

		return nresult;
	}

	strQuery.format("SET NAMES '%s' COLLATE '%s'", m_strCharset.cs(), m_strCollation.cs());
	nr = mysql_query(pHandle, strQuery);
	if ( nr != 0 )  {
		strErr = ::mysql_error(pHandle);
		nErr = ::mysql_errno(pHandle);

		nresult = errMySql2Nr(nErr);

		log_error(L_SQL, "[mysql] can't set collation '%s', result %d, mysql error %d (%s)\n",
				  		m_strCollation.cs(), nresult, nErr, strErr);
		::mysql_close(pHandle);

		return nresult;
	}

	log_trace(L_SQL, "[mysql] successfully connected to %s\n", m_server.cs());

	m_pHandle = pHandle;
	return ESUCCESS;
}

/*
 * Close MySql connection
 *
 * Note: Mutual lock must be held
 */
void CDbMySql::doDisconnect()
{
	shell_assert(m_pHandle);

	::mysql_close(m_pHandle);
	m_pHandle = nullptr;

	log_trace(L_SQL, "[mysql] disconnected database %s\n", m_server.cs());
}

/*
 * Enable/disable multi-statement execution
 *
 * Return:
 * 		ESUCCESS		success
 * 		EIO				failed
 * 		ENOMEM			out of memory error
 */
result_t CDbMySql::enableMultiStatement(boolean_t bEnable)
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	if ( m_bMultiStmt == bEnable )  {
		return ESUCCESS;
	}

	if ( !isConnected() )  {
		m_bMultiStmt = bEnable;
		return ESUCCESS;
	}

	retVal = mysql_set_server_option(m_pHandle, bEnable ?
					MYSQL_OPTION_MULTI_STATEMENTS_ON :
					MYSQL_OPTION_MULTI_STATEMENTS_OFF);
	if ( retVal == 0 )  {
		m_bMultiStmt = bEnable;
		log_trace(L_SQL, "[mysql] multiple statement is %s\n", bEnable ? "enabled" : "disabled");
	}
	else {
		const char*		strErr = ::mysql_error(m_pHandle);
		unsigned int	nErr = ::mysql_errno(m_pHandle);

		if ( nErr == CR_SERVER_GONE_ERROR || nErr == CR_SERVER_LOST ) {
			log_error(L_SQL, "[mysql] %s multiple statement failed, connection failure, "
								"mysql error %d (%s), will repeat on the next connect attempt\n",
					  			bEnable ? "enable" : "disable", nErr, strErr);

			doDisconnect();
		}
		else {
			log_error(L_SQL, "[mysql] %s multiple statement failed, mysql error %d (%s)\n",
					  		bEnable ? "enable" : "disable", nErr, strErr);
			nresult = EIO;
		}
	}

	return nresult;
}

result_t CDbMySql::flushMultiStatementResult()
{
	MYSQL_RES*		pMysqlResult;
	result_t		nresult = ESUCCESS;
	int 			retVal;

	if ( isConnected() )  {
		do {
			pMysqlResult = mysql_store_result(m_pHandle);
			if ( pMysqlResult != nullptr )  {
				mysql_free_result(pMysqlResult);
			}

			retVal = mysql_next_result(m_pHandle);
			if ( retVal > 0 )  {
				const char*		strErr = ::mysql_error(m_pHandle);
				unsigned int	nErr = ::mysql_errno(m_pHandle);

				log_error(L_SQL, "[mysql] getting next result failure, mysql error %d (%s)\n",
						  				nErr, strErr);
				nresult = EIO;
				break;
			}

		} while ( retVal == 0 );
	}

	return nresult;
}

/*
 * Execute a query
 *
 * 		strQuery		query to execute
 *
 * Return:
 * 		ESUCCESS		query executed
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 *
 * Note: Function do connect and reconnect as necessary.
 * Note: Mutual lock must be held
 */
result_t CDbMySql::doQuery(const char* strQuery)
{
	int				nCount;
	result_t		nresult;
	const char*		strErr;
	unsigned int	nErr;

	shell_assert_ex(m_bDbMySqlLibraryInitialised, "MySql library is not "
					  "initialised, call CDbMySql::initLibrary()\n");

	nresult = initThread();
	if ( nresult != ESUCCESS ) {
		return EIO;
	}

	if ( !strQuery || !*strQuery )  {
		log_error(L_SQL, "[mysql] mysql query is empty\n");
		return EIO;
	}

	nresult = EIO;
	nCount = m_nAutoReconnect ? m_nAutoReconnect : 1;

	for(int i=0; i<nCount; i++) {
		if ( !isConnected() )  {
			doConnect();
		}

		if ( isConnected() )  {
			int		rc;

			rc = mysql_query(m_pHandle, strQuery);
			if ( rc == 0 )  {
				/* Success */
				nresult = ESUCCESS;
				break;
			}

			strErr = ::mysql_error(m_pHandle);
			nErr = ::mysql_errno(m_pHandle);
			if ( (nErr != CR_SERVER_GONE_ERROR && nErr != CR_SERVER_LOST) || (i+1) == nCount ) {
				nresult = errMySql2Nr(nErr);

				log_error(L_SQL, "[mysql] query '%s' failed, mysql error %d (%s)\n",
						  			strQuery, nErr, strErr);

				if ( nErr == CR_SERVER_GONE_ERROR || nErr == CR_SERVER_LOST )  {
					doDisconnect();
				}

				break;
			}

			nresult = errMySql2Nr(nErr);

			log_debug(L_SQL, "[mysql] connection lost, trying to reconnect to %s\n",
					  				m_server.cs());
			doDisconnect();
		}
	}

	return nresult;
}

/*
 * Connect to MySql database with predefined parameters
 *
 * Return:
 * 		ESUCCESS		connected
 * 		ENOMEM			out of memory error
 * 		EIO				connection failed
 */
result_t CDbMySql::connect()
{
	CAutoLock		locker(m_lock);
	result_t		nresult;

	nresult = initThread();
	if ( nresult == ESUCCESS && !isConnected() ) {
		nresult = doConnect();
	}

	return nresult;
}

/*
 * Disconnect from MySql database
 *
 * Return:
 * 		ESUCCESS		success
 * 		EIO				general failure
 */
result_t CDbMySql::disconnect()
{
	CAutoLock		locker(m_lock);
	result_t		nresult;

	nresult = initThread();
	if ( nresult == ESUCCESS && isConnected() ) {
		doDisconnect();
	}

	return nresult;
}

/*
 * Execute query
 *
 * 		strQuery		SQL query to execute
 *
 * Return:
 * 		ESUCCESS		query executed
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 *
 * Note: No exception raised.
 */
result_t CDbMySql::query(const char* strQuery)
{
	CAutoLock	locker(m_lock);
	result_t	nresult;

	log_trace(L_SQL, "[mysql] query: '%s'\n", strQuery);

	nresult = doQuery(strQuery);
	return nresult;
}

/*
 * Execute a given SQL query and fetch a string value from the result
 *
 * 		strQuery		SQL query to execute
 * 		pValue			fetched string [out]
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENODATA			query executed but returns no data
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 */
result_t CDbMySql::queryValue(const char* strQuery, CString* pValue)
{
	CAutoLock	locker(m_lock);
	result_t	nresult;

	log_trace(L_SQL, "[mysql] query string value: '%s'\n", strQuery);

	nresult = doQuery(strQuery);
	if ( nresult == ESUCCESS )  {
		MYSQL_RES*		pMysqlResult;
		MYSQL_ROW		pMysqlRow;
		my_ulonglong	nRows;
		unsigned int 	nFields;
		const char*		strErr;
		unsigned int	nErr;

		pMysqlResult = mysql_store_result(m_pHandle);
		if ( pMysqlResult ) {
			nRows = mysql_num_rows(pMysqlResult);
			nFields = mysql_num_fields(pMysqlResult);

			if ( nRows > 0 && nFields > 0 ) {
				pMysqlRow = mysql_fetch_row(pMysqlResult);

				if ( pMysqlRow != nullptr ) {
					*pValue = pMysqlRow[0];
				}
				else {
					strErr = ::mysql_error(m_pHandle);
					nErr = ::mysql_errno(m_pHandle);

					if ( nErr != 0 )  {
						nresult = errMySql2Nr(nErr);
						nresult = nresult == ENOMEM ? ENOMEM : EIO;

						log_error(L_SQL, "[mysql] mysql_fetch_row() failed, query='%s', "
					  						"mysql error %d (%s)\n", strQuery, nErr, strErr);
					}
					else {
						log_debug(L_SQL, "[mysql] fetch row query '%s' returns no data\n",
											strQuery);
						nresult = ENODATA;
					}
				}
			}
			else {
				log_debug(L_SQL, "[mysql] query '%s' returns no data\n", strQuery);
				nresult = ENODATA;
			}

			mysql_free_result(pMysqlResult);
		}
		else {
			strErr = ::mysql_error(m_pHandle);
			nErr = ::mysql_errno(m_pHandle);
			nresult = errMySql2Nr(nErr);
			nresult = nresult == ENOMEM ? ENOMEM : EIO;

			log_error(L_SQL, "[mysql] mysql_store_result() failed, query='%s', "
							 "mysql error %d (%s)\n", strQuery, nErr, strErr);
		}
	}

	return nresult;
}

/*
 * Execute a given SQL query and fetch a UINT64 value from the result
 *
 * 		strQuery		SQL query to execute
 * 		pValue			fetched string [out]
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENODATA			query executed but returns no data
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 */
result_t CDbMySql::queryValue(const char* strQuery, uint64_t* pValue)
{
	CString		strValue;
	uint64_t	nValue;
	result_t	nresult;

	nresult = queryValue(strQuery, &strValue);
	if ( nresult == ESUCCESS )  {
		nresult = strValue.getNumber(nValue);
		if ( nresult == ESUCCESS ) {
			*pValue = nValue;
		}
		else {
			log_debug(L_SQL, "[mysql] invalid uint64 string '%s' in query '%s'\n",
			 				strValue.cs(), strQuery);
		}
	}

	return nresult;
}

/*
 * Execute a given SQL query and fetch a INT64 value from the result
 *
 * 		strQuery		SQL query to execute
 * 		pValue			fetched string [out]
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENODATA			query executed but returns no data
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 */
result_t CDbMySql::queryValue(const char* strQuery, int64_t* pValue)
{
	CString		strValue;
	int64_t		nValue;
	result_t	nresult;

	nresult = queryValue(strQuery, &strValue);
	if ( nresult == ESUCCESS )  {
		nresult = strValue.getNumber(nValue);
		if ( nresult == ESUCCESS ) {
			*pValue = nValue;
		}
		else {
			log_debug(L_SQL, "[mysql] invalid int64 string '%s' in query '%s'\n",
					  strValue.cs(), strQuery);
		}
	}

	return nresult;
}

/*
 * Execute a given SQL query and fetch a UINT32 value from the result
 *
 * 		strQuery		SQL query to execute
 * 		pValue			fetched string [out]
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENODATA			query executed but returns no data
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 */
result_t CDbMySql::queryValue(const char* strQuery, uint32_t* pValue)
{
	CString		strValue;
	uint32_t	nValue;
	result_t	nresult;

	nresult = queryValue(strQuery, &strValue);
	if ( nresult == ESUCCESS )  {
		nresult = strValue.getNumber(nValue);
		if ( nresult == ESUCCESS ) {
			*pValue = nValue;
		}
		else {
			log_debug(L_SQL, "[mysql] invalid uint32 string '%s' in query '%s'\n",
					  strValue.cs(), strQuery);
		}
	}

	return nresult;
}

/*
 * Execute a given SQL query and fetch a INT32 value from the result
 *
 * 		strQuery		SQL query to execute
 * 		pValue			fetched string [out]
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENODATA			query executed but returns no data
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 */
result_t CDbMySql::queryValue(const char* strQuery, int32_t* pValue)
{
	CString		strValue;
	int32_t		nValue;
	result_t	nresult;

	nresult = queryValue(strQuery, &strValue);
	if ( nresult == ESUCCESS )  {
		nresult = strValue.getNumber(nValue);
		if ( nresult == ESUCCESS ) {
			*pValue = nValue;
		}
		else {
			log_debug(L_SQL, "[mysql] invalid int32 string '%s' in query '%s'\n",
					  strValue.cs(), strQuery);
		}
	}

	return nresult;
}

/*
 * Query single row
 *
 * 		strQuery		sql query to execute
 * 		pVector			string values array [out]
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENODATA			query executed but returns no data
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 *
 * Note: NULL fields are returned as empty string ("")
 */
result_t CDbMySql::queryRow(const char* strQuery, str_vector_t* pVector)
{
	CAutoLock	locker(m_lock);
	result_t	nresult;

	shell_assert(pVector);

	log_trace(L_SQL, "[mysql] row query: '%s'\n", strQuery);

	nresult = doQuery(strQuery);
	if ( nresult == ESUCCESS )  {
		MYSQL_RES*		pMysqlResult;
		MYSQL_ROW		pMysqlRow;
		my_ulonglong	nRows;
		unsigned int 	nFields;
		const char		*strErr, *s;
		unsigned int	nErr;

		pMysqlResult = mysql_store_result(m_pHandle);
		if ( pMysqlResult ) {
			nRows = mysql_num_rows(pMysqlResult);
			nFields = mysql_num_fields(pMysqlResult);

			if ( nRows > 0 && nFields > 0 ) {
				pMysqlRow = mysql_fetch_row(pMysqlResult);

				if ( pMysqlRow != nullptr ) {
					try {
						pVector->empty();
						pVector->reserve(nFields);
						for(size_t i=0; i<nFields; i++)  {
							s = pMysqlRow[i];
							pVector->push_back(s != nullptr ? CString(s) : CString());
						}
					}
					catch(const std::bad_alloc& exc)  {
						pVector->empty();
						nresult = ENOMEM;
						log_error(L_SQL, "[mysql] out of memory\n");
					}
					catch(const std::length_error& le) {
						pVector->empty();
						nresult = EIO;
						log_error(L_SQL, "[mysql] invalid vector length\n");
					}
				}
				else {
					strErr = ::mysql_error(m_pHandle);
					nErr = ::mysql_errno(m_pHandle);

					if ( nErr != 0 )  {
						nresult = errMySql2Nr(nErr);

						log_error(L_SQL, "[mysql] mysql_fetch_row() failed, query='%s', "
										 "mysql error %d (%s)\n", strQuery, nErr, strErr);
					}
					else {
						log_debug(L_SQL, "[mysql] fetch row query '%s' returns no data\n",
								  strQuery);
						nresult = ENODATA;
					}
				}
			}
			else {
				log_debug(L_SQL, "[mysql] query '%s' returns no data\n", strQuery);
				nresult = ENODATA;
			}

			mysql_free_result(pMysqlResult);
		}
		else {
			strErr = ::mysql_error(m_pHandle);
			nErr = ::mysql_errno(m_pHandle);
			nresult = errMySql2Nr(nErr);

			log_error(L_SQL, "[mysql] mysql_store_result() failed, query='%s', "
							 "mysql error %d (%s)\n", strQuery, nErr, strErr);
		}
	}

	return nresult;
}

/*
 * Start iterate query
 *
 * 		strQuery		sql query to iterate
 * 		pResult			intermediate result
 *
 * Return: exception on error (nr=EEXIST,ENOENT,EIO,ENOMEM)
 */
void CDbMySql::iterate(const char* strQuery, CSqlResult* pResult) noexcept(false)
{
	CAutoLock		locker(m_lock);
	CMySqlResult*	pMySqlResult = dynamic_cast<CMySqlResult*>(pResult);
	MYSQL_RES*		pMysqlRes;
	const char*		strErr;
	unsigned int	nErr;
	result_t		nresult;

	shell_assert(pMySqlResult);

	log_trace(L_SQL, "[mysql] iterate query: '%s'\n", strQuery);

	nresult = doQuery(strQuery);
	if ( nresult != ESUCCESS )  {
		throw std::sql_exception(nresult);
	}

	pMysqlRes = mysql_store_result(m_pHandle);
	if ( pMysqlRes ) {
		pMySqlResult->init(this, pMysqlRes, FALSE);
		sh_atomic_inc(&m_nResultCount);
	}
	else {
		nErr = ::mysql_errno(m_pHandle);
		if ( nErr != 0 )  {
			strErr = ::mysql_error(m_pHandle);
			nresult = errMySql2Nr(nErr);

			log_error(L_SQL, "[mysql] mysql_store_result() failed, query='%s', "
							 "mysql error %d (%s)\n", strQuery, nErr, strErr);
			throw std::sql_exception(nresult);
		}

		pMySqlResult->init(this, pMysqlRes, FALSE);
	}
}

/*
 * Escape the special characters
 *
 * 		strQuery		string to escape
 * 		strOut			escaped string [out]
 *
 * Return:
 * 		ESUCCESS		query executed
 * 		EIO				query failed
 * 		ENOMEM			out of memory
 */
result_t CDbMySql::escapeSafe(const char* strQuery, CString& strOut)
{
	char*			strBuffer;
	size_t			nLength;
	unsigned long 	nEscLength;
	result_t		nresult;

	int				nCount;
	const char*		strErr;
	unsigned int	nErr;

	shell_assert_ex(m_bDbMySqlLibraryInitialised, "MySql library is not "
					  "initialised, call CDbMySql::initLibrary()\n");

	nresult = initThread();
	if ( nresult != ESUCCESS ) {
		return EIO;
	}

	shell_assert(strQuery);
	nLength = _tstrlen(strQuery);
	if ( !nLength )  {
		strOut.clear();
		return ESUCCESS;
	}

	strBuffer = (char*)memAlloc(nLength*2+1);
	if ( !strBuffer )  {
		log_error(L_SQL, "[mysql] memory allocation failed, size %u\n", nLength*2+1);
		return ENOMEM;
	}

	nresult = EIO;
	nCount = m_nAutoReconnect ? m_nAutoReconnect : 1;

	for(int i=0; i<nCount; i++) {
		if ( !isConnected() )  {
			doConnect();
		}

		if ( isConnected() )  {
			nEscLength = mysql_real_escape_string(m_pHandle, strBuffer, strQuery, nLength);
			if ( nEscLength != (unsigned long)-1 )  {
				/* Success */
				nresult = ESUCCESS;
				break;
			}

			strErr = ::mysql_error(m_pHandle);
			nErr = ::mysql_errno(m_pHandle);
			if ( (nErr != CR_SERVER_GONE_ERROR && nErr != CR_SERVER_LOST) || (i+1) == nCount ) {
				nresult = errMySql2Nr(nErr);

				log_error(L_SQL, "[mysql] escape '%s' failed, mysql error %d (%s)\n",
						  			strQuery, nErr, strErr);

				if ( nErr == CR_SERVER_GONE_ERROR || nErr == CR_SERVER_LOST )  {
					doDisconnect();
				}

				break;
			}

			nresult = EIO;

			log_debug(L_SQL, "[mysql] connection lost, trying to reconnect to %s\n",
					  m_server.cs());
			doDisconnect();
		}
	}

	if ( nresult == ESUCCESS )  {
		try {
			strOut = strBuffer;
		}
		catch(std::bad_alloc& exc) {
			log_error(L_SQL, "[mysql] memory allocation failed, size %lu\n", nEscLength);
			nresult = ENOMEM;
		}
	}

	memFree(strBuffer);
	return nresult;
}

void CDbMySql::escape(const char* strQuery, CString& strOut) noexcept(false)
{
	result_t	nresult;

	nresult = escapeSafe(strQuery, strOut);
	if ( nresult != ESUCCESS )  {
		throw std::sql_exception(nresult);
	}
}

/*
 * Insert a new record to the table
 *
 * 		strQuery		inserting SQL query
 * 		strTable		inserting table name
 * 		pId				inserted record Id
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENODATA			query executed but returns no data
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 */
result_t CDbMySql::recordInsert(const char* strQuery, const char* strTable, uint32_t* pId)
{
	uint64_t	id;
	result_t	nresult;

	nresult = recordInsert(strQuery, strTable, &id);
	if ( nresult == ESUCCESS )  {
		*pId = (uint32_t)id;
	}

	return nresult;
}

/*
 * Insert a new record to the table
 *
 * 		strQuery		inserting SQL query
 * 		strTable		inserting table name
 * 		pId				inserted record Id
 *
 * Return:
 * 		ESUCCESS		success
 * 		ENODATA			query executed but returns no data
 * 		EEXIST			query failed (duplicate key, etc)
 * 		ENOENT			query failed (no such table, etc)
 * 		EIO				query failed (I/O error)
 * 		ENOMEM			out of memory
 */
result_t CDbMySql::recordInsert(const char* strQuery, const char* strTable, uint64_t* pId)
{
	char		strBuf[128];
	uint64_t	id;
	result_t	nresult;

	_tsnprintf(strBuf, sizeof(strBuf), "LOCK TABLES %s WRITE", strTable);
	nresult = query(strBuf);
	if ( nresult == ESUCCESS )  {
		nresult = query(strQuery);
		if ( nresult == ESUCCESS )  {
			_tsnprintf(strBuf, sizeof(strBuf), "SELECT MAX(id) FROM %s", strTable);
			nresult = queryValue(strBuf, &id);
			if ( nresult == ESUCCESS )  {
				*pId = id;
			}
		}

		_tsnprintf(strBuf, sizeof(strBuf), "UNLOCK TABLES");
		query(strBuf);
	}

	return nresult;
}


/*******************************************************************************
 * Debugging support
 */

void CDbMySql::dump(const char* strPref) const
{
	log_dump("*** DbMySql%s: server %s, connected: %d\n",
		  strPref, m_server.cs(), isConnected());
}
