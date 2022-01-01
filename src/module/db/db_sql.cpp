/*
 *  Telemetrica TeleGeo backend server
 *  SQL Database base class
 *
 *  Copyright (c) 2020 Telemetrica. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 03.08.2020 12:02:48
 *      Initial revision.
 */

#include "db/db_sql.h"

/*******************************************************************************
 * CDbSql class
 */

CDbSql::CDbSql(const char* strName) :
	CModule(strName),
	m_hrConnectTimeout(HR_0),
	m_hrSendTimeout(HR_0),
	m_hrRecvTimeout(HR_0)
{
}

CDbSql::~CDbSql()
{
}

/*
 * Set connection maximum time
 *
 * 		hrTimeout		timeout, HR_0 - use default
 */
void CDbSql::setConnectTimeout(hr_time_t hrTimeout)
{
	m_hrConnectTimeout = hrTimeout;
}

/*
 * Set I/O timeouts
 *
 * 		hrSendTimeout			send query timeout, HR_0 - use default
 * 		hrRecvTimeout			receive result timeout, HR_0 - use default
 */
void CDbSql::setTimeouts(hr_time_t hrSendTimeout, hr_time_t hrRecvTimeout)
{
	m_hrSendTimeout = hrSendTimeout;
	m_hrRecvTimeout = hrRecvTimeout;
}

