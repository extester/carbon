/*
 *  Telemetrica TeleGeo backend server
 *  MySQL Query result class
 *
 *  Copyright (c) 2020 Telemetrica. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.08.2020 16:51:35
 *      Initial revision.
 */

#include <mysql/mysql.h>

#include "carbon/logger.h"
#include "db/db_mysql.h"
#include "db/db_mysql_result.h"

/*******************************************************************************
 * CMySqlRow class
 */

char* CMySqlRow::operator[](size_t nIndex) noexcept(false)
{
	if ( nIndex < m_nFields && m_row != nullptr )  {
		return m_row[nIndex];
	}

	log_error(L_SQL, "[sql_row] index %lu overflow, nFields %lu\n", nIndex, m_nFields);
	throw std::sql_exception(EFAULT);
}

uint32_t CMySqlRow::getUint32(size_t nIndex) noexcept(false)
{
	char*		s = (*this)[nIndex];
	uint32_t	n;

	if ( CString(s).getNumber(n) != ESUCCESS )  {
		log_error(L_SQL, "[sql_row] string is not a number: '%s'\n", s);
		throw std::sql_exception(EFAULT);
	}

	return n;
}

int32_t CMySqlRow::getInt32(size_t nIndex) noexcept(false)
{
	char*		s = (*this)[nIndex];
	uint32_t	n;

	if ( CString(s).getNumber(n) != ESUCCESS )  {
		log_error(L_SQL, "[sql_row] string is not a number: '%s'\n", s);
		throw std::sql_exception(EFAULT);
	}

	return n;
}

uint64_t CMySqlRow::getUint64(size_t nIndex) noexcept(false)
{
	char*		s = (*this)[nIndex];
	uint64_t	n;

	if ( CString(s).getNumber(n) != ESUCCESS )  {
		log_error(L_SQL, "[sql_row] string is not a number: '%s'\n", s);
		throw std::sql_exception(EFAULT);
	}

	return n;
}

int64_t CMySqlRow::getInt64(size_t nIndex) noexcept(false)
{
	char*		s = (*this)[nIndex];
	int64_t		n;

	if ( CString(s).getNumber(n) != ESUCCESS )  {
		log_error(L_SQL, "[sql_row] string is not a number: '%s'\n", s);
		throw std::sql_exception(EFAULT);
	}

	return n;
}

/*******************************************************************************
 * CMySqlResult class
 */

/*
 * Get field count in the result rows
 *
 * Return: count
 */
size_t CMySqlResult::getFields() const
{
	return m_pMySqlRes ? (size_t)mysql_num_fields(m_pMySqlRes) : 0;
}

/*
 * Fetch the next row from the query result object
 *
 * 		pRow		row object to place next row
 *
 * Return:
 * 		TRUE		success, returned next row
 * 		FALSE		result is empty, no more rows
 *
 * Note:
 * 		generate std::sql_exception on any db error
 */
boolean_t CMySqlResult::getRow(CSqlRow* pRow) noexcept(false)
{
	CMySqlRow*		pMySqlRow = dynamic_cast<CMySqlRow*>(pRow);
	MYSQL_ROW		row;
	unsigned int	nErr;
	const char* 	strErr;
	result_t		nresult;

	shell_assert(pMySqlRow);

	if ( !m_pMySqlRes )  {
		return FALSE;
	}

	row = mysql_fetch_row(m_pMySqlRes);
	if ( row != nullptr ) {
		pMySqlRow->init(row, mysql_num_fields(m_pMySqlRes));
		return TRUE;
	}

	nErr = ::mysql_errno(m_pDb->m_pHandle);
	if ( nErr != 0 )  {
		strErr = ::mysql_error(m_pDb->m_pHandle);
		nresult = errMySql2Nr(nErr);

		log_error(L_SQL, "[mysql] mysql_fetch_row() failed, mysql error %d (%s)\n",
					  	nErr, strErr);
		throw std::sql_exception(nresult);
	}

	return FALSE;
}

/*
 * Free MySQL query result
 */
void CMySqlResult::free()
{
	if ( m_pMySqlRes )  {
		int32_t		nResultCount;

		mysql_free_result(m_pMySqlRes);
		m_pMySqlRes = nullptr;

		nResultCount = sh_atomic_dec(&m_pDb->m_nResultCount);
		if ( nResultCount < 0 )  {
			log_error(L_SQL, "[mysql] *******************************************\n");
			log_error(L_SQL, "[mysql] invalid query result/free counter: %ld\n", nResultCount);
			log_error(L_SQL, "[mysql] *******************************************\n");
		}
	}
}
