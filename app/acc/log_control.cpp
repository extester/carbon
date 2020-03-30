/*
 *  Carbon Framework Network Client
 *  Log Control page
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 16.09.2015 12:59:30
 *      Initial revision.
 */

#include "shell/logger/logger_base.h"

#include "carbon/carbon.h"

#include "interface.h"
#include "log_control.h"

typedef struct
{
	unsigned int	nChannel;
	char			strChannel[32];
} log_channel_t;

static log_channel_t arLogChannel[] = {
	/* Shell Library */
	{	L_SOCKET,			"L_SOCKET"				},
	{	L_FILE,				"L_FILE"				},
	{	L_MIO,				"L_MIO"					},
	{	L_ICMP,				"L_ICMP"				},
	{	L_EV_TRACE_TIMER,	"L_EV_TRACE_TIMER"		},
	{	L_EV_TRACE_EVENT,	"L_EV_TRACE_EVENT"		},
	{	L_BOOT,				"L_BOOT"				},
	{	L_DB,				"L_DB"					},
	{	L_DB_SQL_TRACE,		"L_DB_SQL_TRACE"		},
	/* Carbon Library */
	{	L_NETCONN,			"L_NETCONN"				},
	{	L_NETCONN_IO,		"L_NETCONN_IO"			},
	{	L_NETSERV,			"L_NETSERV"				},
	{	L_NETSERV_IO,		"L_NETSERV_IO"			},
	{	L_NETCLI,			"L_NETCLI"				},
	{	L_NETCLI_IO,		"L_NETCLI_IO"			}
};

#define LOG_CHANNELS		ARRAY_SIZE(arLogChannel)

CLogControl::CLogControl(CAccInterface* pParent) :
	m_pParent(pParent),
	m_pLabel(0),
	m_bEnabled(FALSE),
	m_bNotify(TRUE)
{
	m_arChannel = new GtkWidget*[LOG_CHANNELS];
}

CLogControl::~CLogControl()
{
	SAFE_DELETE(m_arChannel);
}


GtkWidget* CLogControl::create(const char* strTitle)
{
	GtkWidget	*pGrid, *pWidget;
	size_t		i, lines = (LOG_CHANNELS+1)/2;

	shell_assert(m_pLabel == 0);

	m_bNotify = FALSE;

	pGrid = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(pGrid), 10);
	gtk_grid_set_column_spacing(GTK_GRID(pGrid), 10);
	gtk_container_set_border_width(GTK_CONTAINER(pGrid), 30);

	for(i=0; i<lines; i++) {
		pWidget = gtk_check_button_new_with_label(arLogChannel[i].strChannel);
		g_object_set(pWidget, "xalign", 0.0, NULL);
		gtk_widget_set_size_request(pWidget, 240, -1);
		gtk_widget_set_sensitive(pWidget, FALSE);
		g_signal_connect(G_OBJECT(pWidget), "toggled", G_CALLBACK(onCheckST), this);
		gtk_grid_attach(GTK_GRID(pGrid), pWidget, 0, i, 1, 1);
		m_arChannel[i] = pWidget;

		if ( (i+lines) < LOG_CHANNELS ) {
			pWidget = gtk_check_button_new_with_label(arLogChannel[i+lines].strChannel);
			g_object_set(pWidget, "xalign", 0.0, NULL);
			gtk_widget_set_size_request(pWidget, 240, -1);
			gtk_widget_set_sensitive(pWidget, FALSE);
			g_signal_connect(G_OBJECT(pWidget), "toggled", G_CALLBACK(onCheckST), this);
			gtk_grid_attach(GTK_GRID(pGrid), pWidget, 1, i, 1, 1);
			m_arChannel[i+lines] = pWidget;
		}
	}

	m_pLabel = gtk_label_new(strTitle);
	m_bEnabled = FALSE;
	m_bNotify = TRUE;

	return pGrid;
}

void CLogControl::clear()
{
	if ( m_bEnabled )  {
		size_t		i, lines = (LOG_CHANNELS+1)/2;

		m_bNotify = FALSE;
		for(i=0; i<lines; i++)  {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_arChannel[i]), FALSE);
			gtk_widget_set_sensitive(m_arChannel[i], FALSE);

			if ( (i+lines) < LOG_CHANNELS )  {
				gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_arChannel[i+lines]), FALSE);
				gtk_widget_set_sensitive(m_arChannel[i+lines], FALSE);
			}
		}

		m_bEnabled = FALSE;
		m_bNotify = TRUE;
	}
}

void CLogControl::setLabel(const char* strLabel)
{
	gtk_label_set_text(GTK_LABEL(m_pLabel), strLabel);
}

void CLogControl::enable()
{
	size_t		i, lines = (LOG_CHANNELS+1)/2;

	m_bNotify = FALSE;
	for(i=0; i<lines; i++) {
		gtk_widget_set_sensitive(m_arChannel[i], TRUE);

		if ( (i + lines) < LOG_CHANNELS ) {
			gtk_widget_set_sensitive(m_arChannel[i+lines], TRUE);
		}
	}

	m_bEnabled = TRUE;
	m_bNotify = TRUE;
}

void CLogControl::update(const uint8_t* pChannel)
{
	size_t			i, lines = (LOG_CHANNELS+1)/2;
	unsigned int 	ch, index, bit;

	if ( !m_bEnabled )  {
		enable();
	}

	m_bNotify = FALSE;
	for(i=0; i<lines; i++) {
		ch = GET_CHANNEL0(arLogChannel[i].nChannel);
		index = ch/8;
		bit = 1<<(ch&7);

		shell_assert(index < (LOGGER_CHANNEL_MAX/8));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_arChannel[i]),
									 (pChannel[index]&bit)!=0);

		if ( (i+lines) < LOG_CHANNELS ) {
			ch = GET_CHANNEL0(arLogChannel[i+lines].nChannel);
			index = ch/8;
			bit = 1<<(ch&7);

			shell_assert(index < (LOGGER_CHANNEL_MAX/8));
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_arChannel[i+lines]),
					(pChannel[index]&bit)!=0);
		}
	}
	m_bNotify = TRUE;
}

void CLogControl::onCheck(GtkToggleButton* pCheck)
{
	size_t			i, count = LOG_CHANNELS;
	unsigned int	nChannel = LOGGER_CHANNEL_MAX;
	boolean_t		isOn;

	if ( m_bNotify ) {
		isOn = gtk_toggle_button_get_active(pCheck);

		for (i=0; i<count; i++) {
			if ( m_arChannel[i] == (GtkWidget*) pCheck ) {
				nChannel = arLogChannel[i].nChannel;

				log_debug(L_GEN, "[log_ctl(%s)] log %s is %s\n",
						  gtk_label_get_text(GTK_LABEL(m_pLabel)),
						  arLogChannel[i].strChannel,
						  isOn ? "ENABLED" : "DISABLED");
				break;
			}
		}

		if ( nChannel != LOGGER_CHANNEL_MAX ) {
			m_pParent->onLogControlCheck(nChannel, isOn);
		}
	}
}
