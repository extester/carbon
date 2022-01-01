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

#ifndef __DB_MYSQL_H_INCLUDED__
#define __DB_MYSQL_H_INCLUDED__

#include <mysql/mysql.h>

#include "shell/netaddr.h"
#include "shell/atomic.h"
#include "carbon/cstring.h"

#include "db/db_sql.h"
#include "db/db_mysql_result.h"

/*
 * MySQL database class
 */
class CDbMySql : public CDbSql
{
	friend class CMySqlResult;

	protected:
		CNetAddr		m_server;				/* DB server address */
		CString			m_strUser;				/* User login */
		CString			m_strPass;				/* User password */
		CString			m_strCharset;			/* DB connection charset, default is 'utf8' */
		CString			m_strCollation;			/* DB connection collate, default is 'utf8_general_ci' */
		CString			m_strDb;

		MYSQL*			m_pHandle;				/* Mysql connection handle */
		int				m_nAutoReconnect;		/* 0 - do not use autoreconnect,
 												 * n - use n tries */

		atomic_t 		m_nResultCount;			/* Debugging: calculating result/free ops */

		static boolean_t 			m_bDbMySqlLibraryInitialised;
		static __thread boolean_t 	m_bDbMysqlThreadInitialised;

	public:
		CDbMySql(const CNetAddr& server, const char* strUser, const char* strPass,
		   			 const char* strCharset, const char* strCollation,
		   			 const char* strDb, const char* strName);

		CDbMySql(const CNetAddr& server, const char* strUser, const char* strPass,
					 const char* strDb, const char* strName);

		virtual ~CDbMySql();

		static result_t initLibrary();
		static void terminateLibrary();

		static result_t initThread();
		static void terminateThread();

	public:
		void setAutoReconnect(int nAutoReconnect);

		virtual result_t connect();
		virtual result_t disconnect();
		virtual	boolean_t isConnected() const { return m_pHandle != nullptr; }

		virtual result_t query(const char* strQuery);

		virtual result_t queryValue(const char* strQuery, CString* pString);
		virtual result_t queryValue(const char* strQuery, uint64_t* pValue);
		virtual result_t queryValue(const char* strQuery, int64_t* pValue);
		virtual result_t queryValue(const char* strQuery, uint32_t* pValue);
		virtual result_t queryValue(const char* strQuery, int32_t* pValue);

		virtual result_t queryRow(const char* strQuery, str_vector_t* pVector);

		virtual void iterate(const char* strQuery, CSqlResult* pResult) noexcept(false);

		virtual result_t escapeSafe(const char* strQuery, CString& strOut);
		virtual void escape(const char* strQuery, CString& strOut) noexcept(false);

		virtual result_t recordInsert(const char* strQuery, const char* strTable, uint32_t* pId);
		virtual result_t recordInsert(const char* strQuery, const char* strTable, uint64_t* pId);

	protected:
		result_t doConnect();
		void doDisconnect();
		result_t doQuery(const char* strQuery);

	public:
		virtual void dump(const char* strPref = "") const;
};

extern result_t errMySql2Nr(unsigned int nErr);

#endif /* __DB_MYSQL_H_INCLUDED__ */
