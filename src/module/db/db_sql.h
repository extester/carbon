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
 *  Revision 1.0, 01.08.2015 19:41:12
 *      Initial revision.
 */

#ifndef __CARBON_DB_SQL_H_INCLUDED__
#define __CARBON_DB_SQL_H_INCLUDED__

#include <vector>

#include "db/db.h"

/*
 * Supported SQL types
 */
typedef enum {
	SQL_VT_EMPTY,					/* Empty type */
	SQL_VT_NULL,					/* Null type */
	SQL_VT_INTEGER,					/* 64 bits */
	SQL_VT_REAL,					/* 8 bytes */
	SQL_VT_TEXT,					/* 2**31-1 */
	SQL_VT_BLOB,					/* 2**31-1 */
	SQL_VT_ERROR
} SQL_VARIANT_TYPE;

/*
 * SQL column variable
 */
typedef struct {
	SQL_VARIANT_TYPE	vtype;
	uint32_t			vsize;

	union {
		uint64_t		valInt;
		double			valDouble;
		const char*		valText;
		const void*		valBlob;
	};
} SQL_VARIANT;


/*
 * Base class represents a temporary single row result
 */
class CSqlResult
{
	protected:
		CSqlResult() {}
	public:
		virtual ~CSqlResult() {}

	public:
		virtual size_t getComumns() const = 0;
		virtual SQL_VARIANT operator[](int index) const = 0;
		virtual result_t getNextRow() = 0;
};


/*
 * Class contains a single SQL row
 */
class CSqlRow
{
	protected:
		std::vector<SQL_VARIANT>		m_arComumn;

	public:
		CSqlRow();
		virtual ~CSqlRow();

	public:
		size_t getComumns() const { return m_arComumn.size(); }
		SQL_VARIANT operator[](int index) const;

		result_t create(CSqlResult* pResult);

	private:
		void clear();
};


class CDbSql : public CDb
{
	protected:
		CDbSql(const char* strObject) : CDb(strObject) {}
	public:
		virtual ~CDbSql() {}

	public:
		virtual result_t connect() = 0;
		virtual void disconnect() = 0;

		virtual result_t query(const char* strQuery, CSqlResult** ppResult) = 0;
		virtual void escape(const char* strInput, char* strOutput, size_t szOutput) = 0;

		virtual result_t iterateNext(CSqlResult* pResult);
		virtual void iterateEnd(CSqlResult* pResult);
};

#endif /* __CARBON_DB_SQL_H_INCLUDED__ */
