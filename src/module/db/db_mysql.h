/*
 *  Carbon/DB module
 *  Mysql database class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 28.07.2015 23:20:18
 *      Initial revision.
 */

#ifndef __CARBON_DB_MYSQL_H_INCLUDED__
#define __CARBON_DB_MYSQL_H_INCLUDED__

#include <mysql/mysql.h>

#include "carbon/carbon.h"

#include "db/db.h"

class CDbMysql : public CDb
{
	private:
		static __thread boolean_t 	m_bThreadInitialised;
		static boolean_t			m_bLibraryInitialised;

	protected:
		MYSQL*			m_pMysqlConn;

	public:
		CDbMysql(const char* strName = "db-mysql");
		virtual ~CDbMysql();

	public:
		static result_t libraryInit();
		static void libraryExit();

		static result_t threadInit();
		static void threadExit();

		virtual result_t init();
		virtual void terminate();

	protected:
		void checkThreadInit() const {
			shell_assert(m_bThreadInitialised);
		}
};

#endif /* __CARBON_DB_MYSQL_H_INCLUDED__ */
