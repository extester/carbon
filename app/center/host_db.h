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
 *  Revision 1.0, 09.08.2015 15:39:01
 *      Initial revision.
 */

#ifndef __HOST_DB_H_INCLUDED__
#define __HOST_DB_H_INCLUDED__

#include "shell/netaddr.h"
#include "db/db_sqlite.h"
#include "db/db_sql_table.h"

#include "carbon/carbon.h"

#include "packet_center.h"

#define HOST_TABLE      "host"

class CHostTable : public CDbSqlTable
{
    public:
        CHostTable(CDbSql* pDb);
        virtual ~CHostTable();

    public:
        result_t createTable();
        result_t insertHost(ip_addr_t ipHost, const char* strHost, host_id_t* pId);
        result_t updateHost(host_id_t id, ip_addr_t ipHost, const char* strHost);
        result_t deleteHost(host_id_t id);
};


#endif /* __HOST_DB_H_INCLUDED__ */
