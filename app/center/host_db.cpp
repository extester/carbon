/*
 *  Carbon Framework Network Center
 *  Host list database
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 09.08.2015 15:41:17
 *      Initial revision.
 */
/*
 * Table 'host'
 *
 * 	id				INT UNSIGNED NOT NULL PRIMARY			host ID
 * 	name			CHAR(64) NOT NULL DEFAULT ''			host name
 * 	ip				INT UNSIGNED NOT NULL DEFAULT 0			host ip
 *
 */

#include "host_db.h"


CHostTable::CHostTable(CDbSql* pDb) :
	CDbSqlTable(pDb, HOST_TABLE)
{
	shell_assert(pDb);
}

CHostTable::~CHostTable()
{
	SAFE_DELETE(m_pDb);
}

/*
 * Create host table
 *
 * Return: ESUCCESS, ...
 */
result_t CHostTable::createTable()
{
	const char* strQueryFmt =
		"CREATE TABLE IF NOT EXISTS %s ("
		"id		INTEGER PRIMARY KEY AUTOINCREMENT, "
		"name	CHAR(64) NOT NULL 		DEFAULT '', "
		"ip		INT UNSIGNED NOT NULL 	DEFAULT '0')";
	char 		strQuery[256];
	result_t	nresult;

	snprintf(strQuery, sizeof(strQuery), strQueryFmt, HOST_TABLE);
	nresult = query(strQuery);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[host_db] failed to create table %s, result: %d\n", HOST_TABLE, nresult);
	}

	return nresult;
}

/*
 * Insert new host
 *
 * 		ipHost		host IP address
 *		strHost		host name
 *		pId			host ID [out]
 *
 *	Return: ESUCCESS, ...
 */
result_t CHostTable::insertHost(ip_addr_t ipHost, const char* strHost, host_id_t* pId)
{
	char		*strQuery, *strHostNameEscaped;
	size_t		ln;
	result_t	nresult;

	shell_assert(ipHost);
	shell_assert(strHost && *strHost);
	shell_assert(pId);

	nresult = connect();
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[host_tab] failed to connect to DB, result: %d\n", nresult);
		return nresult;
	}

	ln = strlen(strHost);
	strHostNameEscaped = (char*)alloca(ln*2+1);
	escape(strHost, strHostNameEscaped, ln*2+1);
	ln = strlen(strHostNameEscaped);

	strQuery = (char*)alloca(ln+getSzTable()+128);

	snprintf(strQuery, ln+getSzTable()+128,
			 "INSERT INTO %s (name, ip) VALUES('%s', %u)",
			 getTable(), strHostNameEscaped, (uint32_t)ipHost);
	nresult = recordInsert(strQuery, pId);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[host_tab] failed to insert host '%s', ip: %u, result: %d\n",
				  strHost, ipHost, nresult);
	}

	disconnect();

	return nresult;
}

/*
 * Update host parameters
 *
 * 		id			host ID to update
 * 		ipHost		new host IP address (may be 0)
 *		strHost		new host name (may be NULL)
 *
 *	Return: ESUCCESS, ...
 */
result_t CHostTable::updateHost(host_id_t id, ip_addr_t ipHost, const char* strHost)
{
	char		*strQuery, *strHostEscaped = 0;
	size_t		ln = 0, l;
	int			n;
	boolean_t	b = FALSE;
	result_t	nresult;

	if ( ipHost == 0 || strHost == 0 )  {
		return ESUCCESS;		/* Nothing to update */
	}

	nresult = connect();
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[host_tab] failed to connect to DB, result: %d\n", nresult);
		return nresult;
	}

	if ( strHost != 0 ) {
		ln = strlen(strHost);
		strHostEscaped = (char*) alloca(ln * 2 + 1);
		escape(strHost, strHostEscaped, ln * 2 + 1);
		ln = strlen(strHostEscaped);
	}

	l = ln + getSzTable() + 128;
	strQuery = (char*)alloca(l);

	n = snprintf(strQuery, l, "UPDATE %s SET ", getTable());
	if ( ipHost != 0 )  {
		n += snprintf(&strQuery[n], l-n, "ip=%u", ipHost);
		b = TRUE;
	}
	if ( strHost != 0 )  {
		n += snprintf(&strQuery[n], l-n, "%s name='%s'", b ? "," : "", strHostEscaped);
	}

	snprintf(&strQuery[n], l-n, " WHERE id=%u", id);

	nresult = query(strQuery);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[host_tab] failed to update host id: %u, result: %d\n",
				  id, nresult);
	}

	disconnect();

	return nresult;
}

/*
 * Delete host
 *
 * 		id		host Id to delete
 *
 * Return: ESUCCESS, ...
 */
result_t CHostTable::deleteHost(host_id_t id)
{
	result_t	nresult;

	nresult = connect();
	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[host_tab] failed to connect to DB, result: %d\n", nresult);
		return nresult;
	}

	nresult = recordDelete(id);
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[host_db] failed to delete host, id %u, result: %d\n",
				  id, nresult);
	}

	disconnect();

	return nresult;
}

