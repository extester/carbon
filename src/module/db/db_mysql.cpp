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
 *  Revision 1.0, 28.07.2015 23:22:03
 *      Initial revision.
 */
/*
 * MySQL Initialisation
 *		nresult = CDbMysql::libraryInit();
 *    	if ( nresult != ESUCCESS )  {
 *      	::exit(1);
 *    	}
 *    	nresult = CDbMysql::threadInit();
 *    	shell_assert(nresult == ESUCCESS); UNUSED(nresult);
 *
 * MySQL termination:
 *
 *		CDbMysql::threadExit();
 *		CDbMysql::libraryExit();
 *
 */

#include "carbon/carbon.h"

#include "db/db_mysql.h"

#define ALWAYS_USE_THREADED_LIBRARY		1

/*******************************************************************************/

__thread boolean_t CDbMysql::m_bThreadInitialised = FALSE;
		 boolean_t CDbMysql::m_bLibraryInitialised = FALSE;

CDbMysql::CDbMysql(const char* strName) :
	CDb(strName),
	m_pMysqlConn(0)

{
	shell_assert(m_bLibraryInitialised);

#if ALWAYS_USE_THREADED_LIBRARY
	if ( !mysql_thread_safe() ) {
		shell_assert(mysql_thread_safe());
		log_error(L_GEN, "[mysql] MYSQL client library is NOT thread-safe!\n");
		::exit(1);
	}
#endif /* ALWAYS_USE_THREADED_LIBRARY */
}

CDbMysql::~CDbMysql()
{
}

result_t CDbMysql::libraryInit()
{
	result_t	nresult = ESUCCESS;

	shell_assert(!m_bLibraryInitialised);
	if ( !m_bLibraryInitialised )  {
		int 	retVal;

		retVal = mysql_library_init(0, 0, NULL);
		if ( !retVal )  {
			m_bLibraryInitialised = TRUE;
		}
		else {
			log_error(L_GEN, "[mysql] failed to init MySQL library!\n");
			nresult = EFAULT;
		}
	}

	return nresult;
}

void CDbMysql::libraryExit()
{
	if ( m_bLibraryInitialised )  {
		mysql_library_end();
		m_bLibraryInitialised = FALSE;
	}
}

result_t CDbMysql::threadInit()
{
	result_t	nresult = ESUCCESS;

	if ( !m_bThreadInitialised )  {
		int		retVal;

		retVal = mysql_thread_init();
		shell_assert(retVal == 0);
		if ( !retVal ) {
			m_bThreadInitialised = TRUE;
		}
		else {
			log_error(L_GEN, "[mysql] thread initialisation FAILED!\n");
			nresult = EFAULT;
		}
	}

	return nresult;
}

void CDbMysql::threadExit()
{
	if ( m_bThreadInitialised )  {
		mysql_thread_end();
		m_bThreadInitialised = FALSE;
	}
}

result_t CDbMysql::init()
{
	shell_assert(m_bLibraryInitialised);
	shell_assert(m_bThreadInitialised);
	shell_assert(m_pMysqlConn == 0);

	m_pMysqlConn = mysql_init(NULL);
	if ( !m_pMysqlConn )  {
		log_error(L_GEN, "[mysql] failed to init library, ENOMEM\n");
		return ENOMEM;
	}

	return ESUCCESS;
}

void CDbMysql::terminate()
{
	if ( m_pMysqlConn )  {
		mysql_close(m_pMysqlConn);
		m_pMysqlConn = 0;
	}
}
