/*
 *  Telemetrica TeleGeo backend server
 *  SQL Database base class
 *
 *  Copyright (c) 2020 Telemetrica. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.08.2020 12:00:38
 *      Initial revision.
 */

#ifndef __DB_SQL_H_INCLUDED__
#define __DB_SQL_H_INCLUDED__

#include <stdexcept>

#include "shell/shell.h"
#include "shell/lock.h"

#include "carbon/module.h"
#include "carbon/cstring.h"
#include "carbon/utils.h"


namespace std {

class sql_exception : public exception
{
	private:
		result_t	m_nr;

	public:
		sql_exception(result_t nr) throw() :
			exception(), m_nr(nr) {}

		virtual ~sql_exception() throw() {}

	public:
		virtual result_t getResult() const { return m_nr; }
		virtual const char* what() const throw() { return ""; }
};

}; /* std */

/*
 * Helper base class for SQL row iterations
 */
class CSqlRow
{
	public:
		CSqlRow() {}
		virtual ~CSqlRow() {}

	public:
		virtual char* operator[](size_t nIndex) noexcept(false) /*throw(std::exception)*/ = 0;

		virtual uint32_t getUint32(size_t nIndex) noexcept(false) = 0;
		virtual int32_t getInt32(size_t nIndex) noexcept(false) = 0;
		virtual uint64_t getUint64(size_t nIndex) noexcept(false) = 0;
		virtual int64_t getInt64(size_t nIndex) noexcept(false) = 0;
};

/*
 * Base class represents a query result
 */
class CSqlResult
{
	public:
		CSqlResult() {}
		virtual ~CSqlResult() {}

	public:
		virtual size_t getFields() const = 0;
		virtual boolean_t getRow(CSqlRow* pRow) noexcept(false) = 0;
		virtual void free() = 0;
};

/*******************************************************************************
 * SQL Database base class
 */
class CDbSql : public CModule
{
	protected:
		CMutex			m_lock;
		hr_time_t 		m_hrConnectTimeout;
		hr_time_t 		m_hrSendTimeout;
		hr_time_t		m_hrRecvTimeout;

	public:
		CDbSql(const char* strName);
		virtual ~CDbSql();

	public:
		virtual void setConnectTimeout(hr_time_t hrTimeout);
		virtual void setTimeouts(hr_time_t hrSendTimeout = HR_0, hr_time_t hrRecvTimeout = HR_0);

		virtual boolean_t isConnected() const = 0;
		virtual result_t query(const char* strQuery) = 0;

		virtual result_t queryValue(const char* strQuery, CString* pValue) = 0;
		virtual result_t queryValue(const char* strQuery, uint64_t* pValue) = 0;
		virtual result_t queryValue(const char* strQuery, int64_t* pValue) = 0;
		virtual result_t queryValue(const char* strQuery, uint32_t* pValue) = 0;
		virtual result_t queryValue(const char* strQuery, int32_t* pValue) = 0;
		virtual result_t queryRow(const char* strQuery, str_vector_t* pVector) = 0;
		virtual void iterate(const char* strQuery, CSqlResult* pResult) noexcept(false) = 0;

	public:
		virtual void dump(const char* strPref = "") const = 0;
};

#endif /* __DB_SQL_H_INCLUDED__ */
