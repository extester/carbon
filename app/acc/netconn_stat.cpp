/*
 *  Carbon Framework Network Client
 *  Connector Statistic page
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 15.09.2015-13 17:03:39
 *      Initial revision.
 */

#include <gtk/gtk.h>

#include "netconn_stat.h"

CNetConnStat::CNetConnStat() :
	m_pIncomClientCount(0),
	m_pIncomFailCount(0),
	m_pRecvCount(0),
	m_pSendCount(0),
	m_pIOCount(0),
	m_pIOFailCount(0),
	m_pPoolCount(0),
	m_pLabel(0)
{
}

CNetConnStat::~CNetConnStat()
{
}

void CNetConnStat::clear()
{
	if ( m_pIncomClientCount )  {
		gtk_label_set_text(GTK_LABEL(m_pIncomClientCount), "");
	}
	if ( m_pIncomFailCount )  {
		gtk_label_set_text(GTK_LABEL(m_pIncomFailCount), "");
	}
	if ( m_pRecvCount )  {
		gtk_label_set_text(GTK_LABEL(m_pRecvCount), "");
	}
	if ( m_pSendCount )  {
		gtk_label_set_text(GTK_LABEL(m_pSendCount), "");
	}
	if ( m_pIOFailCount )  {
		gtk_label_set_text(GTK_LABEL(m_pIOFailCount), "");
	}
	if ( m_pPoolCount )  {
		gtk_label_set_text(GTK_LABEL(m_pPoolCount), "");
	}
}

GtkWidget* CNetConnStat::create(const char* strTitle)
{
	GtkWidget	*pGrid, *pWidget;
	size_t		i, count;
	struct { const char* strName; GtkWidget** pp; } items[] = {
		{	"Incoming Clients:",		&m_pIncomClientCount	},
		{	"Incoming Client Fails:",	&m_pIncomFailCount		},
		{	"Received Packets:",		&m_pRecvCount			},
		{	"Sent Packets:",			&m_pSendCount			},
		{	"I/O Request Fails:",		&m_pIOFailCount			},
		{	"Worker Pool Threads:",		&m_pPoolCount			}
	};

	shell_assert(m_pLabel == 0);

	pGrid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID(pGrid), 10);
	gtk_grid_set_column_spacing (GTK_GRID(pGrid), 10);
	gtk_container_set_border_width(GTK_CONTAINER(pGrid), 30);

	count = ARRAY_SIZE(items);
	for(i=0; i<count; i++) {
		pWidget = gtk_label_new(items[i].strName);
		g_object_set(pWidget, "xalign", 0.0, NULL);
		gtk_widget_set_size_request(pWidget, 240, -1);
		gtk_grid_attach(GTK_GRID(pGrid), pWidget, 0, i, 1, 1);

		*items[i].pp = gtk_label_new("");
		g_object_set(*items[i].pp, "xalign", 0.0, NULL);
		gtk_widget_set_size_request(*items[i].pp, 240, -1);
		gtk_grid_attach(GTK_GRID(pGrid), *items[i].pp, 1, i, 1, 1);
	}

	m_pLabel = gtk_label_new(strTitle);
	return pGrid;
}

void CNetConnStat::update(const tcpconn_stat_t* pStat)
{
	char	strTmp[64];

	shell_assert(m_pLabel != 0);
	if ( m_pLabel == 0 )  {
		return;
	}

	snprintf(strTmp, sizeof(strTmp), "%d", counter_get(pStat->client));
	gtk_label_set_text(GTK_LABEL(m_pIncomClientCount), strTmp);

	snprintf(strTmp, sizeof(strTmp), "%d", counter_get(pStat->client_fail));
	gtk_label_set_text(GTK_LABEL(m_pIncomFailCount), strTmp);

	snprintf(strTmp, sizeof(strTmp), "%d", counter_get(pStat->recv));
	gtk_label_set_text(GTK_LABEL(m_pRecvCount), strTmp);

	snprintf(strTmp, sizeof(strTmp), "%d", counter_get(pStat->send));
	gtk_label_set_text(GTK_LABEL(m_pSendCount), strTmp);

	snprintf(strTmp, sizeof(strTmp), "%d", counter_get(pStat->fail));
	gtk_label_set_text(GTK_LABEL(m_pIOFailCount), strTmp);

	snprintf(strTmp, sizeof(strTmp), "%d", counter_get(pStat->worker));
	gtk_label_set_text(GTK_LABEL(m_pPoolCount), strTmp);
}

void CNetConnStat::setLabel(const char* strLabel)
{
	gtk_label_set_text(GTK_LABEL(m_pLabel), strLabel);
}
