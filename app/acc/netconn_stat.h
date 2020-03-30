/*
 *  Carbon Framework Network Client
 *  Connector Statistic page
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 15.09.2015-13 16:49:24
 *      Initial revision.
 */

#ifndef __NETCONN_STAT_H_INCLUDED__
#define __NETCONN_STAT_H_INCLUDED__

#include <gtk/gtk.h>

#include "carbon/net_connector/tcp_connector.h"

class CAccInterface;

#define NETCONN_STAT_DISABLED				(-1)
#define NETCONN_STAT_LOCAL					0
#define NETCONN_STAT_CENTER					1
#define NETCONN_STAT_EXTERNAL				2

#define NETCONN_STAT_INTERVAL				HR_2SEC

class CNetConnStat
{
	private:
		GtkWidget*		m_pIncomClientCount;
		GtkWidget*		m_pIncomFailCount;
		GtkWidget*		m_pRecvCount;
		GtkWidget*		m_pSendCount;
		GtkWidget*		m_pIOCount;
		GtkWidget*		m_pIOFailCount;
		GtkWidget*		m_pPoolCount;

		GtkWidget*		m_pLabel;

	public:
		CNetConnStat();
		~CNetConnStat();

	public:
		GtkWidget* create(const char* strTitle);
		void update(const tcpconn_stat_t* pStat);
		void clear();

		GtkWidget* getLabel() const { return m_pLabel; }
		void setLabel(const char* strLabel);
};


#endif /* __NETCONN_STAT_H_INCLUDED__ */