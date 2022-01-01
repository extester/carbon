/*
 *  Telemetrica TeleGeo backend server
 *  MySQL Query result class
 *
 *  Copyright (c) 2020 Telemetrica. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.08.2020 16:49:58
 *      Initial revision.
 */

#ifndef __DB_MYSQL_RESULT_H_INCLUDED__
#define __DB_MYSQL_RESULT_H_INCLUDED__

#include <mysql/mysql.h>

#include "db/db_sql.h"

/*
 * Helper class for MySQL row iterations
 */
class CMySqlRow : public CSqlRow
{
	protected:
		MYSQL_ROW 			m_row;			/* Current row or nullptr */
		size_t				m_nFields;		/* Fields in the row */

	public:
		CMySqlRow() : CSqlRow(), m_row(nullptr), m_nFields(0) {}
		virtual ~CMySqlRow() {}

	public:
		virtual void init(MYSQL_ROW row, size_t nFields) {
			m_row = row; m_nFields = nFields;
		}

		virtual char* operator[](size_t nIndex) noexcept(false);

		virtual uint32_t getUint32(size_t nIndex) noexcept(false);
		virtual int32_t getInt32(size_t nIndex) noexcept(false);
		virtual uint64_t getUint64(size_t nIndex) noexcept(false);
		virtual int64_t getInt64(size_t nIndex) noexcept(false);
};

class CDbMySql;

/*
 * Class represents a MySQL query result
 */
class CMySqlResult : public CSqlResult
{
	protected:
		CDbMySql*		m_pDb;
		MYSQL_RES*		m_pMySqlRes;
		boolean_t		m_bLocked;

	public:
		CMySqlResult() : CSqlResult(), m_pDb(nullptr), m_pMySqlRes(nullptr), m_bLocked(FALSE) {}
		virtual ~CMySqlResult() { free(); }

	public:
		virtual size_t getFields() const;
		virtual boolean_t getRow(CSqlRow* pRow) noexcept(false);

		virtual void init(CDbMySql* pDb, MYSQL_RES* pMySqlRes, boolean_t bLocked) {
			shell_assert(pDb);
			m_pDb = pDb;
			m_pMySqlRes = pMySqlRes;
			m_bLocked = bLocked;
		}

		virtual void free();
};

#endif /* __DB_MYSQL_RESULT_H_INCLUDED__ */
