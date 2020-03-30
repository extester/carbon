/*
 *  Carbon Framework Network Client
 *  GTK Interface
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 14.07.2015 16:27:00
 *      Initial revision.
 */

#include "carbon/carbon.h"
#include "carbon/config_xml.h"

#include "acc.h"
#include "../center/packet_center.h"
#include "interface.h"

#define HOST_COUNT_MAX			64

enum {
	COLUMN_NAME,
	COLUMN_IP,
	COLUMN_TIME,
	COLUMN_ID,
	COLUMN_COUNT
};

CAccInterface::CAccInterface(CAccApp* pParent) :
	CModule("AccInterface"),
	m_pParent(pParent),
	m_thInterface("interface"),
	m_pApp(0),
	m_pMainWindow(0),
	m_pOnOff(0),
	m_pUpdateButton(0),
	m_pDeleteButton(0),
	m_pListView(0),
	m_pNetState(0),
	m_pBusyState(0),

	m_pMemStatDialog(0),
	m_nMemStatType(MEMORY_STAT_DISABLED),

	m_pNetConnStatDialog(0),
	m_nNetConnStatType(NETCONN_STAT_DISABLED),

	m_pLogCtlDialog(0),
	m_nLogCtlType(LOG_CONTROL_DISABLED)
{
	m_arHost.reserve(HOST_COUNT_MAX);

	_tbzero_object(m_arNetState);
	_tbzero_object(m_arMemStat);
	_tbzero_object(m_arNetConnStat);
	_tbzero_object(m_arLogCtl);

	m_memStatExtAddr = CNetAddr("127.0.0.1");
	m_netconnStatExtAddr = CNetAddr("127.0.0.1");
	m_logctlExtAddr = CNetAddr("127.0.0.1");
}

CAccInterface::~CAccInterface()
{
	size_t	i, count;

	count = ARRAY_SIZE(m_arMemStat);
	for(i=0; i<count; i++)  {
		SAFE_DELETE(m_arMemStat[i]);
	}

	if ( m_pApp )  {
		g_object_unref(m_pApp);
		m_pApp = 0;
	}

	if ( m_arNetState[0] )  {
		g_object_unref(m_arNetState[0]);
		m_arNetState[0] = 0;
	}

	if ( m_arNetState[1] )  {
		g_object_unref(m_arNetState[1]);
		m_arNetState[1] = 0;
	}
}

void CAccInterface::formatValueMs(hr_time_t hrTime, char* strBuf, size_t length)
{
	if ( hrTime != ICMP_ITER_FAILED && hrTime != ICMP_ITER_NOTSET ) {
		if ( hrTime < HR_1SEC ) {
			snprintf(strBuf, length, "%.3f ms",
					 ((double)HR_TIME_TO_MICROSECONDS(hrTime))/1000 );
		}
		else {
			snprintf(strBuf, length, "%.3f sec",
					 ((double)HR_TIME_TO_MILLISECONDS(hrTime))/1000 );
		}
	}
	else {
		snprintf(strBuf, length, hrTime == ICMP_ITER_FAILED ? "-failed-" : "-notset-");
	}
}

host_item_t* CAccInterface::createHost(host_id_t id, const char* strName,
									   ip_addr_t ipAddr, hr_time_t hrPing)
{
	host_item_t		host;

	_tbzero_object(host);
	host.id = id;
	host.ipaddr = CNetHost(ipAddr);
	copyString(host.strName, strName, sizeof(host.strName));
	host.hrPing = hrPing;

	m_arHost.push_back(host);
	return &m_arHost[m_arHost.size()-1];
}

/*
 * Add a new host to the host list
 *
 * 		index			host index in the array
 *
 */
void CAccInterface::addHostRow(int index)
{
	host_item_t*	pItem = &m_arHost[index];
	GtkTreeIter		iterator;
	GtkListStore*	pListStore;
	char 			strTmp[32];

	shell_assert(index < (int)m_arHost.size());
	shell_assert(m_pListView);

	pListStore = (GtkListStore*)gtk_tree_view_get_model(GTK_TREE_VIEW(m_pListView));

	formatValueMs(pItem->hrPing, strTmp, sizeof(strTmp));

	gtk_list_store_append(pListStore, &iterator);
	gtk_list_store_set(pListStore, &iterator,
					   	COLUMN_ID, pItem->id,
						COLUMN_NAME, pItem->strName,
						COLUMN_IP, (const char*)CNetHost(pItem->ipaddr),
						COLUMN_TIME, strTmp,
						-1);
}

void CAccInterface::addHost(host_id_t id, const char* strName, ip_addr_t ipAddr, hr_time_t hrPing)
{
	if ( m_pListView != 0 && m_arHost.size() < HOST_COUNT_MAX )  {
		createHost(id, strName, ipAddr, hrPing);
		addHostRow((int)m_arHost.size()-1);
	}
}

int CAccInterface::findHost(host_id_t id) const
{
	int		index = -1;
	size_t	i, count;

	count = m_arHost.size();
	for(i=0; i<count; i++)  {
		if ( m_arHost[i].id == id )  {
			index = (int)i;
			break;
		}
	}

	return index;
}

void CAccInterface::updateHostTime(int index)
{
	host_item_t*	pItem = &m_arHost[index];
	GtkListStore*	pListStore;
	GtkTreeIter		iterator;
	char			strTmp[32];
	boolean_t		bResult;

	shell_assert(index < (int)m_arHost.size());
	shell_assert(m_pListView);

	pListStore = (GtkListStore*)gtk_tree_view_get_model(GTK_TREE_VIEW(m_pListView));

	snprintf(strTmp, sizeof(strTmp), "%u", index);
	bResult = gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(pListStore), &iterator, strTmp);
	if ( bResult ) {
		formatValueMs(pItem->hrPing, strTmp, sizeof(strTmp));
		gtk_list_store_set(pListStore, &iterator, COLUMN_TIME, strTmp, -1);
	}
	else {
		log_error(L_GEN, "[acc_interface] failed to get row %d\n", index);
	}
}

void CAccInterface::updateHost(host_id_t id, const char* strName, ip_addr_t ipAddr, hr_time_t hrPing)
{
	int 	index;

	gdk_threads_enter();

	index = findHost(id);
	if ( index >= 0 ) {
		/*
		 * Update existing host data
		 */
		if ( m_arHost[index].hrPing != hrPing ) {
			m_arHost[index].hrPing = hrPing;
			updateHostTime(index);
		}
	}
	else {
		/*
		 * Add new host to the table
		 */
		addHost(id, strName, ipAddr, hrPing);
	}

	gdk_threads_leave();
}

/*
 * Delete host by index in the table
 *
 * 		index			host index to delete
 *
 * Note: This is private function without locking
 * to be called within interface thread. Otherwise
 * call to gdk_threads_enter() is required.
 */
void CAccInterface::deleteHost(int index)
{
	host_item_t*	pItem = &m_arHost[index];
	GtkListStore*	pListStore;
	GtkTreeIter		iterator;
	char			strTmp[32];
	boolean_t		bResult;

	shell_assert(index < (int)m_arHost.size());
	shell_assert(m_pListView);

	log_debug(L_GEN, "[acc_iface] deleting host id: %u, name: %s\n", pItem->id, pItem->strName);


	pListStore = (GtkListStore*)gtk_tree_view_get_model(GTK_TREE_VIEW(m_pListView));

	snprintf(strTmp, sizeof(strTmp), "%u", index);
	bResult = gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(pListStore), &iterator, strTmp);
	if ( bResult ) {
		gtk_list_store_remove(pListStore, &iterator);
		m_arHost.erase(m_arHost.begin()+index);
	}
	else {
		log_error(L_GEN, "[acc_interface] failed to get row %d\n", index);
	}
}

void CAccInterface::activateHandler()
{
	GtkWidget			*mainBox, *ha, *box, *toolbar;
	GtkToolItem			*button;
	GdkPixbuf*			pixBuf;
	char				stmp[128];
	GError*				error = NULL;

	GtkListStore* 		pListStore;
	GtkCellRenderer*	pRenderer;
	GtkTreeViewColumn*	pColumn;

	CConfig*			pConfig = m_pParent->getConfig();
	int					x, y;

	shell_assert(m_pApp);
	shell_assert(m_pMainWindow == 0);
	shell_assert(m_pOnOff == 0);
	shell_assert(m_pListView == 0);

	/*
	 * Main window
	 */
	snprintf(stmp, sizeof(stmp), "%s %u.%u", ACC_NAME, VERSION_MAJOR(ACC_VERSION),
			 VERSION_MINOR(ACC_VERSION));

	m_pMainWindow = gtk_application_window_new (m_pApp);
	gtk_window_set_title (GTK_WINDOW (m_pMainWindow), stmp);
	gtk_window_set_resizable(GTK_WINDOW(m_pMainWindow), false);
	if ( pConfig->get("main_window/x", &x) == ESUCCESS &&
			pConfig->get("main_window/y", &y) == ESUCCESS ) {
		gtk_window_move(GTK_WINDOW(m_pMainWindow), x, y);
	}
	else {
		gtk_window_set_position(GTK_WINDOW(m_pMainWindow), GTK_WIN_POS_CENTER);
	}
	g_signal_connect(G_OBJECT(m_pMainWindow), "configure-event",
					 G_CALLBACK(onConfigureMainWindowST), this);

	/*
	 * Network states
	 */

	m_arNetState[0] = gdk_pixbuf_new_from_file("offline.png", &error);
	if ( !m_arNetState[0] )  {
		log_error(L_GEN, "[acc_interface] failed to load network offline image\n");
	}
	m_arNetState[1] = gdk_pixbuf_new_from_file("online.png", &error);
	if ( !m_arNetState[1] )  {
		log_error(L_GEN, "[acc_interface] failed to load network online image\n");
	}

	/*
	 * Main container
	 */
	mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width(GTK_CONTAINER(mainBox), 5);
	g_object_set(mainBox, "margin-right", 15, NULL);
	g_object_set(mainBox, "margin-left", 15, NULL);
	gtk_container_add(GTK_CONTAINER (m_pMainWindow), mainBox);

	/*
	 * Toolbar
	 */
	toolbar = gtk_toolbar_new();

	button = gtk_tool_button_new_from_stock(GTK_STOCK_NEW);
	gtk_tool_item_set_tooltip_text(button, "New host");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, -1);
	g_signal_connect(G_OBJECT(button), "clicked",
					 G_CALLBACK(onNewHostST), this);

	m_pUpdateButton = gtk_tool_button_new_from_stock(GTK_STOCK_EDIT);
	gtk_tool_item_set_tooltip_text(m_pUpdateButton, "Edit selected host");
	gtk_widget_set_sensitive(GTK_WIDGET(m_pUpdateButton), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), m_pUpdateButton, -1);
	g_signal_connect(G_OBJECT(m_pUpdateButton), "clicked",
					 G_CALLBACK(onUpdateHostST), this);

	m_pDeleteButton = gtk_tool_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_tool_item_set_tooltip_text(m_pDeleteButton, "Delete selected host");
	gtk_widget_set_sensitive(GTK_WIDGET(m_pDeleteButton), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), m_pDeleteButton, -1);
	g_signal_connect(G_OBJECT(m_pDeleteButton), "clicked",
					 G_CALLBACK(onDeleteHostST), this);

	button = gtk_tool_button_new_from_stock(GTK_STOCK_PROPERTIES);
	gtk_tool_item_set_tooltip_text(button, "Memory Info");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, -1);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(onMemoryStatDialogST), this);

	button = gtk_tool_button_new_from_stock(GTK_STOCK_NETWORK);
	gtk_tool_item_set_tooltip_text(button, "Network Connector Info");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, -1);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(onNetConnStatDialogST), this);

	button = gtk_tool_button_new_from_stock(GTK_STOCK_PASTE);
	gtk_tool_item_set_tooltip_text(button, "Logger Control");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, -1);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(onLogControlDialogST), this);

	button = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
	gtk_tool_item_set_tooltip_text(button, "Exit");
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), button, -1);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(onQuitST), this);

	gtk_container_add(GTK_CONTAINER(mainBox), toolbar);

	/* 'On/Off' switch */
	ha = gtk_alignment_new(1, 1, 0, 0);
	gtk_container_add(GTK_CONTAINER(mainBox), ha);

	m_pOnOff = gtk_switch_new();
	gtk_switch_set_active(GTK_SWITCH(m_pOnOff), TRUE);
	gtk_container_add(GTK_CONTAINER(ha), m_pOnOff);
	g_signal_connect(m_pOnOff, "notify::active", G_CALLBACK(onOffHandlerST), this);

	/* Host list */
	pListStore = gtk_list_store_new(COLUMN_COUNT, G_TYPE_STRING, G_TYPE_STRING,
									G_TYPE_STRING, G_TYPE_INT);

	m_pListView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pListStore));

	/* Column 'Name' */
	pRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(m_pListView),
												 -1,
												 "Host",
												 pRenderer,
												 "text", COLUMN_NAME,
												 NULL);

	pColumn = gtk_tree_view_get_column(GTK_TREE_VIEW(m_pListView), COLUMN_NAME);

	gtk_tree_view_column_set_resizable(pColumn, TRUE);
	gtk_tree_view_column_set_sizing(pColumn, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_min_width(pColumn, 200);

	/* Column 'Ip' */
	pRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(m_pListView),
												 -1,
												 "Ip address",
												 pRenderer,
												 "text", COLUMN_IP,
												 NULL);

	pColumn = gtk_tree_view_get_column(GTK_TREE_VIEW(m_pListView), COLUMN_IP);

	gtk_tree_view_column_set_resizable(pColumn, TRUE);
	gtk_tree_view_column_set_sizing(pColumn, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_min_width(pColumn, 160);


	/* Column 'Time' */
	pRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW (m_pListView),
												 -1,
												 "Time",
												 pRenderer,
												 "text", COLUMN_TIME,
												 NULL);

	pColumn = gtk_tree_view_get_column(GTK_TREE_VIEW(m_pListView), COLUMN_TIME);

	gtk_tree_view_column_set_resizable(pColumn, TRUE);
	gtk_tree_view_column_set_sizing(pColumn, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	gtk_tree_view_column_set_min_width(pColumn, 150);

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_pListView));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect(G_OBJECT(selection), "changed",
					 G_CALLBACK(onSelectionST), this);

	g_object_unref(pListStore);
	gtk_container_add(GTK_CONTAINER(mainBox), m_pListView);


	/* State bar */
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	g_object_set(box, "margin-top", 10, NULL);
	g_object_set(box, "margin-bottom", 5, NULL);
	g_object_set(box, "spacing", 5, NULL);
	gtk_container_add(GTK_CONTAINER(mainBox), box);

	m_pNetState = gtk_image_new_from_pixbuf(m_arNetState[0]);
	gtk_container_add(GTK_CONTAINER(box), m_pNetState);

	pixBuf = gdk_pixbuf_new_from_file("working.png", &error);
	if ( pixBuf ) {
		m_pBusyState = gtk_image_new_from_pixbuf(pixBuf);
		gtk_container_add(GTK_CONTAINER(box), m_pBusyState);
	}
	else {
		log_error(L_GEN, "[acc_interface] failed to load BUSY image\n");
	}

	m_pCenterVersion = gtk_label_new("Apollo Center: is not connected");
	gtk_container_add(GTK_CONTAINER(mainBox), m_pCenterVersion);

	gtk_widget_show_all (m_pMainWindow);
	if ( m_pBusyState )  {
		gtk_widget_hide(m_pBusyState);
	}
}

void CAccInterface::onConfigureMainWindow(GtkWindow* pWindow, GdkEvent* pEvent)
{
	//GdkEventConfigure*	pCfg = (GdkEventConfigure*)pEvent;
	CConfig*	pConfig = m_pParent->getConfig();
	gint 		wx, wy;

	gtk_window_get_position(pWindow, &wx, &wy);

	//gdk_window_get_origin (gtk_widget_get_window(GTK_WIDGET(pWindow)), &wx, &wy);

	pConfig->set("main_window/x", wx);
	pConfig->set("main_window/y", wy);

	//log_debug(L_GEN, "ev: %d - %d, origin: %d - %d\n", pCfg->x, pCfg->y, wx, wy);
}


boolean_t CAccInterface::onOffHandler()
{
	CEvent*		pEvent;

	gboolean	bState;

	g_object_get(GTK_SWITCH(m_pOnOff), "active", &bState, NULL);

	pEvent = new CEvent(EV_HB_ENABLE, m_pParent, 0,
						(NPARAM)bState, bState ? "enable" : "disable");
	appSendEvent(pEvent);

	return FALSE;
}

boolean_t CAccInterface::doHostDialog(const char* strTitle, const char* strButton,
								 char* strHost, char* strIp)
{
	GtkWidget			*table, *label;
	GtkWidget			*wdtDialog, *wdtMsgDialog;
	GtkWidget			*wdtIp, *wdtName;
	CNetHost			ipHost;
	gint				response;
	boolean_t			bResult = FALSE;

	wdtDialog = gtk_dialog_new_with_buttons (strTitle,
		GTK_WINDOW(m_pMainWindow),
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		strButton,
		GTK_RESPONSE_OK,
		"Cancel",
		GTK_RESPONSE_CANCEL,
		NULL);

	gtk_container_set_border_width(GTK_CONTAINER(wdtDialog), 5);

	table = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(table), 2);
	gtk_grid_set_column_spacing(GTK_GRID(table), 70);

	label = gtk_label_new("Host Name:");
	g_object_set(label, "xalign", 0.0, NULL);
	gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);
	gtk_widget_show(label);

	wdtName = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY(wdtName), HOSTLIST_NAME_MAX-1);
	gtk_entry_set_text(GTK_ENTRY(wdtName), strHost);
	gtk_grid_attach(GTK_GRID(table), wdtName, 1, 0, 1, 1);
	gtk_widget_show(wdtName);

	label = gtk_label_new("Host Ip:");
	g_object_set(label, "xalign", 0.0, NULL);
	gtk_grid_attach(GTK_GRID(table), label, 0, 1, 1, 1);
	gtk_widget_show(label);

	wdtIp = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(table), wdtIp, 1, 1, 1, 1);
	gtk_entry_set_max_length (GTK_ENTRY(wdtIp), 15);
	gtk_entry_set_text(GTK_ENTRY(wdtIp), strIp);
	gtk_widget_show(wdtIp);

	gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(wdtDialog))),
						table, TRUE, TRUE, 0);
	gtk_widget_show(table);

	response = gtk_dialog_run(GTK_DIALOG(wdtDialog));
	if (  response == GTK_RESPONSE_OK )  {
		copyString(strHost, gtk_entry_get_text(GTK_ENTRY(wdtName)), HOSTLIST_NAME_MAX);
		copyString(strIp, gtk_entry_get_text(GTK_ENTRY(wdtIp)), 16);

		ipHost.setHost(strIp);
		if ( strlen(strHost) == 0 )  {
			wdtMsgDialog = gtk_message_dialog_new (GTK_WINDOW(wdtDialog),
												   (GtkDialogFlags)GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_CLOSE,
												   "Missing host name!");
			gtk_dialog_run(GTK_DIALOG (wdtMsgDialog));
			gtk_widget_destroy(wdtMsgDialog);
		} else
		if ( !ipHost.isValid() || strchr(strIp, '.') == NULL )  {
			wdtMsgDialog = gtk_message_dialog_new (GTK_WINDOW(wdtDialog),
												   (GtkDialogFlags)GTK_DIALOG_DESTROY_WITH_PARENT,
												   GTK_MESSAGE_ERROR,
												   GTK_BUTTONS_CLOSE,
												   "Invalid IP address '%s'!", strIp);
			gtk_dialog_run(GTK_DIALOG (wdtMsgDialog));
			gtk_widget_destroy(wdtMsgDialog);
		} else {
			bResult = TRUE;
		}
	}

	gtk_widget_destroy(wdtDialog);

	return bResult;
}

void CAccInterface::onNewHost(GtkToolButton* widget)
{
	char 		strHost[HOSTLIST_NAME_MAX] = "", strIp[16] = "";
	boolean_t	bResult;

	bResult = doHostDialog("Insert New Host", "OK", strHost, strIp);
	if ( bResult )  {
		insert_host_data_t	data;
		CEventInsertHost*	pEvent;

		copyString(data.strHost, strHost, sizeof(data.strHost));
		data.ipHost.setHost(strIp);

		pEvent = new CEventInsertHost(&data, m_pParent);
		appSendEvent(pEvent);
	}
}

void CAccInterface::onUpdateHost(GtkToolButton* widget)
{
	GtkTreeSelection*	pSelection;
	GtkTreeModel*		pListStore;
	GtkTreeIter       	iterator;

	pListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(m_pListView));

	/* !!! This will only work in single or browse selection mode !!! */
	pSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_pListView));

	if ( gtk_tree_selection_get_selected(pSelection, &pListStore, &iterator) ) {
		host_id_t	id;
		int 		index;

		gtk_tree_model_get(pListStore, &iterator, COLUMN_ID, &id, -1);

		index = findHost(id);
		if ( index >= 0 ) {
			char 				strHost[HOSTLIST_NAME_MAX], strIp[16];
			host_item_t* 		pItem = &m_arHost[index];
			char				strTmp[32];
			boolean_t			bResult;
			update_host_data_t	data;
			CEventUpdateHost*	pEvent;

			copyString(strHost, pItem->strName, sizeof(strHost));
			copyString(strIp, (const char*)pItem->ipaddr, sizeof(strIp));

			bResult = doHostDialog("Edit Host Parameters", "Save", strHost, strIp);
			if ( bResult ) {
				index = findHost(id);
				if ( index >= 0 ) {
					pItem = &m_arHost[index];
					copyString(pItem->strName, strHost, sizeof(pItem->strName));
					pItem->ipaddr.setHost(strIp);

					snprintf(strTmp, sizeof(strTmp), "%u", index);
					bResult = gtk_tree_model_get_iter_from_string(pListStore, &iterator, strTmp);
					if ( bResult ) {
						gtk_list_store_set(GTK_LIST_STORE(pListStore), &iterator,
										   COLUMN_NAME, strHost,
										   COLUMN_IP, strIp,
										   -1);
					}

					data.id = id;
					copyString(data.strHost, strHost, sizeof(data.strHost));
					data.ipHost.setHost(strIp);

					pEvent = new CEventUpdateHost(&data, m_pParent);
					appSendEvent(pEvent);
				}
			}
		}
	}
}


void CAccInterface::onDeleteHost(GtkToolButton* widget)
{
	GtkTreeSelection*	pSelection;
	GtkTreeModel*		pListStore;
	GtkTreeIter       	iterator;

	pListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(m_pListView));

	/* !!! This will only work in single or browse selection mode !!! */
	pSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_pListView));

	if ( gtk_tree_selection_get_selected(pSelection, &pListStore, &iterator) ) {
		host_id_t	id;
		int 		index;

		gtk_tree_model_get(pListStore, &iterator, COLUMN_ID, &id, -1);
		index = findHost(id);

		if ( index >= 0 ) {
			host_item_t* 	item = &m_arHost[index];
			GtkWidget* 		wdtMsgDialog;
			gint 			response;

			wdtMsgDialog = gtk_message_dialog_new_with_markup(
					GTK_WINDOW(m_pMainWindow),
					(GtkDialogFlags) GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					"Do you want to delete host <b>%s</b>?",
					item->strName);

			response = gtk_dialog_run(GTK_DIALOG(wdtMsgDialog));
			gtk_widget_destroy(wdtMsgDialog);

			if ( response == GTK_RESPONSE_YES ) {
				CEvent* 	pEvent;
				char 		strTmp[32];

				snprintf(strTmp, sizeof(strTmp), "id=%u", id);
				pEvent = new CEvent(EV_HOST_REMOVE, m_pParent, 0, (NPARAM) id, strTmp);
				appSendEvent(pEvent);
				deleteHost(index);
			}
		}
	}
}

void CAccInterface::updateMemoryStat(int type, const memory_stat_t* pStat)
{
	shell_assert(type != MEMORY_STAT_DISABLED);

	gdk_threads_enter();
	if ( m_pMemStatDialog && m_nMemStatType == type )  {
		m_arMemStat[type]->update(pStat);
	}
	gdk_threads_leave();
}

void CAccInterface::doMemoryStat(int type, const CNetHost& extAddr)
{
	if ( type != m_nMemStatType )  {
		CEvent*		pEvent;
		char		strTmp[64];

		m_nMemStatType = type;

		if ( type != MEMORY_STAT_EXTERNAL )  {
			snprintf(strTmp, sizeof(strTmp), "type=%d", type);
		}
		else {
			snprintf(strTmp, sizeof(strTmp), "type%d, addr=%s\n", type, (const char*)extAddr);
		}

		pEvent = new CEvent(EV_MEMORY_STAT, m_pParent, (PPARAM)(size_t)(ip_addr_t)extAddr,
				(NPARAM)type, strTmp);
		appSendEvent(pEvent);
	}
}

void CAccInterface::onMemoryStatDialog(GtkToolButton* widget)
{
	GtkWidget	*notebook, *grid;
	char 		strTmp[32];

	if ( m_pMemStatDialog )  {
		onMemoryStatDialogResponse(GTK_DIALOG(m_pMemStatDialog), GTK_RESPONSE_OK);
		return;		/* Dialog is already visible */
	}

	m_arMemStat[0] = new CMemoryStat;
	m_arMemStat[1] = new CMemoryStat;
	m_arMemStat[2] = new CMemoryStat;
	m_nMemStatType = MEMORY_STAT_DISABLED;

	m_pMemStatDialog = gtk_dialog_new_with_buttons("Memory Telemetry",
		GTK_WINDOW(m_pMainWindow),
		(GtkDialogFlags)GTK_DIALOG_DESTROY_WITH_PARENT,
		"Set address...",
		GTK_RESPONSE_APPLY,
		"Close",
		GTK_RESPONSE_OK,
		NULL);

	g_signal_connect(m_pMemStatDialog, "response", G_CALLBACK (onMemoryStatDialogResponseST), this);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(m_pMemStatDialog), GTK_RESPONSE_APPLY, FALSE);

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

	grid = m_arMemStat[0]->create("Local");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, m_arMemStat[0]->getLabel());

	grid = m_arMemStat[1]->create("Center");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, m_arMemStat[1]->getLabel());

	snprintf(strTmp, sizeof(strTmp), "External [%s]", (const char*)m_memStatExtAddr);
	grid = m_arMemStat[2]->create(strTmp);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, m_arMemStat[2]->getLabel());

	gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(m_pMemStatDialog))),
						notebook, TRUE, TRUE, 0);

	gtk_widget_show_all(m_pMemStatDialog);
	doMemoryStat(MEMORY_STAT_LOCAL, NETHOST_NULL);

	/* Must be the last, otherwise every gtk_notebook_append_page() generates the signals */
	g_signal_connect(notebook, "switch-page", G_CALLBACK (onMemoryStatPageST), this);
}

void CAccInterface::onMemoryStatPage(int nPage)
{
	doMemoryStat(nPage, m_memStatExtAddr);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(m_pMemStatDialog), GTK_RESPONSE_APPLY,
									   nPage == MEMORY_STAT_EXTERNAL);
}

void CAccInterface::onMemoryStatDialogResponse(GtkDialog* pDialog, int responseId)
{
	if ( pDialog == (GtkDialog*)m_pMemStatDialog )  {
		if ( responseId == GTK_RESPONSE_OK ) {
			gtk_widget_destroy(m_pMemStatDialog);
			m_pMemStatDialog = 0;

			SAFE_DELETE(m_arMemStat[0]);
			SAFE_DELETE(m_arMemStat[1]);
			SAFE_DELETE(m_arMemStat[2]);

			doMemoryStat(MEMORY_STAT_DISABLED, NETHOST_NULL);
		}
		else {
			shell_assert(m_nMemStatType == MEMORY_STAT_EXTERNAL);

			if ( runDialogExtAddress(pDialog, m_memStatExtAddr) ) {
				char 	strTmp[32];

				snprintf(strTmp, sizeof(strTmp), "External [%s]", (const char*) m_memStatExtAddr);
				m_arMemStat[MEMORY_STAT_EXTERNAL]->setLabel(strTmp);

				doMemoryStat(MEMORY_STAT_DISABLED, NETHOST_NULL);
				m_arMemStat[MEMORY_STAT_EXTERNAL]->clear();

				doMemoryStat(MEMORY_STAT_EXTERNAL, m_memStatExtAddr);
			}
		}
	}
}

void CAccInterface::updateNetConnStat(int type, const tcpconn_stat_t* pStat)
{
	shell_assert(type != NETCONN_STAT_DISABLED);

	gdk_threads_enter();
	if ( m_pNetConnStatDialog && m_nNetConnStatType == type )  {
		m_arNetConnStat[type]->update(pStat);
	}
	gdk_threads_leave();
}

void CAccInterface::doNetConnStat(int type, const CNetHost& extAddr)
{
	if ( type != m_nNetConnStatType )  {
		CEvent*		pEvent;
		char		strTmp[64];

		m_nNetConnStatType = type;

		if ( type != NETCONN_STAT_EXTERNAL )  {
			snprintf(strTmp, sizeof(strTmp), "type=%d", type);
		}
		else {
			snprintf(strTmp, sizeof(strTmp), "type%d, addr=%s\n", type, (const char*)extAddr);
		}

		pEvent = new CEvent(EV_NETCONN_STAT, m_pParent, (PPARAM)(size_t)(ip_addr_t)extAddr,
				(NPARAM)type, strTmp);
		appSendEvent(pEvent);
	}
}

void CAccInterface::onNetConnStatDialog(GtkToolButton* widget)
{
	GtkWidget	*notebook, *grid;
	char 		strTmp[32];

	if ( m_pNetConnStatDialog )  {
		onNetConnStatDialogResponse(GTK_DIALOG(m_pNetConnStatDialog), GTK_RESPONSE_OK);
		return;		/* Dialog is already visible */
	}

	m_arNetConnStat[0] = new CNetConnStat;
	m_arNetConnStat[1] = new CNetConnStat;
	m_arNetConnStat[2] = new CNetConnStat;
	m_nNetConnStatType = NETCONN_STAT_DISABLED;

	m_pNetConnStatDialog = gtk_dialog_new_with_buttons("Network Connector Telemetry",
		GTK_WINDOW(m_pMainWindow),
		(GtkDialogFlags)GTK_DIALOG_DESTROY_WITH_PARENT,
		"Set address...",
		GTK_RESPONSE_APPLY,
		"Close",
		GTK_RESPONSE_OK,
		NULL);

	g_signal_connect(m_pNetConnStatDialog, "response", G_CALLBACK (onNetConnStatDialogResponseST), this);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(m_pNetConnStatDialog), GTK_RESPONSE_APPLY, FALSE);

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

	grid = m_arNetConnStat[0]->create("Local");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, m_arNetConnStat[0]->getLabel());

	grid = m_arNetConnStat[1]->create("Center");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, m_arNetConnStat[1]->getLabel());

	snprintf(strTmp, sizeof(strTmp), "External [%s]", (const char*)m_netconnStatExtAddr);
	grid = m_arNetConnStat[2]->create(strTmp);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, m_arNetConnStat[2]->getLabel());

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(m_pNetConnStatDialog))),
						notebook, TRUE, TRUE, 0);

	gtk_widget_show_all(m_pNetConnStatDialog);
	doNetConnStat(NETCONN_STAT_LOCAL, NETHOST_NULL);

	/* Must be the last, otherwise every gtk_notebook_append_page() generates the signals */
	g_signal_connect(notebook, "switch-page", G_CALLBACK (onNetConnStatPageST), this);
}

void CAccInterface::onNetConnStatPage(int nPage)
{
	doNetConnStat(nPage, m_netconnStatExtAddr);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(m_pNetConnStatDialog), GTK_RESPONSE_APPLY,
									  nPage == NETCONN_STAT_EXTERNAL);
}

boolean_t CAccInterface::runDialogExtAddress(GtkDialog* pParent, CNetHost& extAddr)
{
	GtkWidget	*pDialog, *table, *label, *edit;
	gint		responseId;
	CNetHost	tmp;
	boolean_t	bResult = FALSE;

	pDialog = gtk_dialog_new_with_buttons ("External IP",
				GTK_WINDOW(pParent),
				(GtkDialogFlags)(GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT),
				"OK", GTK_RESPONSE_OK,
				"Cancel", GTK_RESPONSE_CANCEL,
				NULL);

	gtk_container_set_border_width(GTK_CONTAINER(pDialog), 5);

	table = gtk_grid_new();
	gtk_grid_set_row_spacing(GTK_GRID(table), 2);
	gtk_grid_set_column_spacing(GTK_GRID(table), 70);

	label = gtk_label_new("Host Ip:");
	g_object_set(label, "xalign", 0.0, NULL);
	gtk_grid_attach(GTK_GRID(table), label, 0, 0, 1, 1);

	edit = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY(edit), 15);
	gtk_entry_set_text(GTK_ENTRY(edit), (const char*)extAddr);
	gtk_grid_attach(GTK_GRID(table), edit, 1, 0, 1, 1);

	gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(pDialog))),
						table, TRUE, TRUE, 0);
	gtk_widget_show_all(table);

	responseId = gtk_dialog_run(GTK_DIALOG(pDialog));
	if (  responseId == GTK_RESPONSE_OK ) {
		tmp.setHost(gtk_entry_get_text(GTK_ENTRY(edit)));
		if ( tmp.isValid() )  {
			extAddr = tmp;
			bResult = TRUE;
		}
	}
	gtk_widget_destroy(pDialog);

	return bResult;
}

void CAccInterface::onNetConnStatDialogResponse(GtkDialog* pDialog, int responseId)
{
	if ( pDialog == (GtkDialog*)m_pNetConnStatDialog )  {
		if ( responseId == GTK_RESPONSE_OK ) {
			gtk_widget_destroy(m_pNetConnStatDialog);
			m_pNetConnStatDialog = 0;

			SAFE_DELETE(m_arNetConnStat[0]);
			SAFE_DELETE(m_arNetConnStat[1]);
			SAFE_DELETE(m_arNetConnStat[2]);

			doNetConnStat(NETCONN_STAT_DISABLED, NETHOST_NULL);
		}
		else {
			shell_assert(m_nNetConnStatType == NETCONN_STAT_EXTERNAL);

			if ( runDialogExtAddress(pDialog, m_netconnStatExtAddr) ) {
				char 	strTmp[32];

				snprintf(strTmp, sizeof(strTmp), "External [%s]", (const char*) m_netconnStatExtAddr);
				m_arNetConnStat[NETCONN_STAT_EXTERNAL]->setLabel(strTmp);

				doNetConnStat(NETCONN_STAT_DISABLED, NETHOST_NULL);
				m_arNetConnStat[NETCONN_STAT_EXTERNAL]->clear();

				doNetConnStat(NETCONN_STAT_EXTERNAL, m_netconnStatExtAddr);
			}
		}
	}
}

/*******************************************************************************
 * LogControl
 */

void CAccInterface::updateLogControl(int type, const uint8_t* pChannel)
{
	shell_assert(type != LOG_CONTROL_DISABLED);

	gdk_threads_enter();
	if ( m_pLogCtlDialog && m_nLogCtlType == type )  {
		m_arLogCtl[type]->update(pChannel);
	}
	gdk_threads_leave();
}

void CAccInterface::doLogControl(int type, const CNetHost& extAddr)
{
	if ( type != m_nNetConnStatType )  {
		CEvent*		pEvent;
		char		strTmp[64];

		m_nLogCtlType = type;

		if ( type != LOG_CONTROL_EXTERNAL )  {
			snprintf(strTmp, sizeof(strTmp), "type=%d", type);
		}
		else {
			snprintf(strTmp, sizeof(strTmp), "type%d, addr=%s\n", type, (const char*)extAddr);
		}

		pEvent = new CEvent(EV_LOG_CONTROL, m_pParent, (PPARAM)(size_t)(ip_addr_t)extAddr,
				(NPARAM)type, strTmp);
		appSendEvent(pEvent);
	}
}

void CAccInterface::onLogControlDialog(GtkToolButton* widget)
{
	GtkWidget	*notebook, *grid;
	char 		strTmp[32];

	if ( m_pLogCtlDialog )  {
		onLogControlDialogResponse(GTK_DIALOG(m_pLogCtlDialog), GTK_RESPONSE_OK);
		return;		/* Dialog is already visible */
	}

	m_arLogCtl[0] = new CLogControl(this);
	m_arLogCtl[1] = new CLogControl(this);
	m_arLogCtl[2] = new CLogControl(this);
	m_nLogCtlType = LOG_CONTROL_DISABLED;

	m_pLogCtlDialog = gtk_dialog_new_with_buttons(
			"Logger Control",
			GTK_WINDOW(m_pMainWindow),
			(GtkDialogFlags)GTK_DIALOG_DESTROY_WITH_PARENT,
			"Set address...",
			GTK_RESPONSE_APPLY,
			"Close",
			GTK_RESPONSE_OK,
			NULL);

	g_signal_connect(m_pLogCtlDialog, "response", G_CALLBACK(onLogControlDialogResponseST), this);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(m_pLogCtlDialog), GTK_RESPONSE_APPLY, FALSE);

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

	grid = m_arLogCtl[0]->create("Local");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, m_arLogCtl[0]->getLabel());

	grid = m_arLogCtl[1]->create("Center");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, m_arLogCtl[1]->getLabel());

	snprintf(strTmp, sizeof(strTmp), "External [%s]", (const char*)m_logctlExtAddr);
	grid = m_arLogCtl[2]->create(strTmp);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), grid, m_arLogCtl[2]->getLabel());

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(m_pLogCtlDialog))),
					   notebook, TRUE, TRUE, 0);

	gtk_widget_show_all(m_pLogCtlDialog);
	doLogControl(LOG_CONTROL_LOCAL, NETHOST_NULL);

	/* Must be the last, otherwise every gtk_notebook_append_page() generates the signals */
	g_signal_connect(notebook, "switch-page", G_CALLBACK (onLogControlPageST), this);
}

void CAccInterface::onLogControlDialogResponse(GtkDialog* pDialog, int responseId)
{
	if ( pDialog == (GtkDialog*)m_pLogCtlDialog )  {
		if ( responseId == GTK_RESPONSE_OK ) {
			gtk_widget_destroy(m_pLogCtlDialog);
			m_pLogCtlDialog = 0;

			SAFE_DELETE(m_arLogCtl[0]);
			SAFE_DELETE(m_arLogCtl[1]);
			SAFE_DELETE(m_arLogCtl[2]);

			//doLogControl(LOG_CONTROL_DISABLED, NETHOST_NULL);
		}
		else {
			shell_assert(m_nLogCtlType == LOG_CONTROL_EXTERNAL);

			if ( runDialogExtAddress(pDialog, m_logctlExtAddr) ) {
				char 	strTmp[32];

				snprintf(strTmp, sizeof(strTmp), "External [%s]", (const char*) m_logctlExtAddr);
				m_arLogCtl[LOG_CONTROL_EXTERNAL]->setLabel(strTmp);

				//doNetConnStat(NETCONN_STAT_DISABLED, NETHOST_NULL);
				m_arLogCtl[LOG_CONTROL_EXTERNAL]->clear();
				doLogControl(LOG_CONTROL_EXTERNAL, m_logctlExtAddr);
			}
		}
	}
}

void CAccInterface::onLogControlPage(int nPage)
{
	doLogControl(nPage, m_logctlExtAddr);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(m_pLogCtlDialog), GTK_RESPONSE_APPLY,
									  nPage == LOG_CONTROL_EXTERNAL);
}


void CAccInterface::onLogControlCheck(unsigned int nChannel, boolean_t isOn)
{
	CEvent*		pEvent;
	char		strTmp[64];

	snprintf(strTmp, sizeof(strTmp), "levChan=%Xh, isOn=%d", nChannel, (int)isOn);
	pEvent = new CEvent(EV_LOG_CONTROL_CHECK, m_pParent, (PPARAM)(size_t)nChannel,
						(NPARAM)isOn, strTmp);
	appSendEvent(pEvent);
}


void CAccInterface::onQuit(GtkToolButton* widget)
{
	if ( m_pMainWindow ) {
		g_signal_emit_by_name(m_pMainWindow, "destroy");
	}
}

void CAccInterface::onSelection(GtkTreeSelection* pSelection)
{
	GtkTreeModel*	pListStore;
	GtkTreeIter		iterator;
	gboolean		bSelected = FALSE;

	pListStore = gtk_tree_view_get_model(GTK_TREE_VIEW(m_pListView));
	if ( gtk_tree_selection_get_selected(pSelection, &pListStore, &iterator) ) {
		bSelected = TRUE;
	}

	gtk_widget_set_sensitive(GTK_WIDGET(m_pUpdateButton), bSelected);
	gtk_widget_set_sensitive(GTK_WIDGET(m_pDeleteButton), bSelected);
}


void CAccInterface::updateNetState(boolean_t bOnline)
{
	gdk_threads_enter();
	if ( m_pNetState )  {
		gtk_image_set_from_pixbuf(GTK_IMAGE(m_pNetState), m_arNetState[bOnline ? 1 : 0]);
	}
	gdk_threads_leave();
}

void CAccInterface::updateCenterVersion(const char* strVersion)
{
	gdk_threads_enter();
	if ( m_pNetState )  {
		char 	stmp[128];

		snprintf(stmp, sizeof(stmp), "Apollo Center: %s", strVersion);
		gtk_label_set_text(GTK_LABEL(m_pCenterVersion), stmp);
	}
	gdk_threads_leave();
}

void CAccInterface::updateBusyState(boolean_t bShow)
{
	gdk_threads_enter();
	if ( m_pBusyState )  {
		if ( bShow )  {
			gtk_widget_show(m_pBusyState);
		}
		else {
			gtk_widget_hide(m_pBusyState);
		}
	}
	gdk_threads_leave();
}

/*
 * Interface worker thread
 */
void* CAccInterface::thread(CThread* pThread, void* pData)
{
	pThread->bootCompleted(ESUCCESS);

	if( !g_thread_supported() ) {
		g_thread_init(NULL);
	}

	gdk_threads_init();		/* Secure gtk */

	//gdk_threads_enter();
	m_pApp = gtk_application_new("org.apollonet.acc", G_APPLICATION_FLAGS_NONE);

	g_signal_connect(m_pApp, "activate", G_CALLBACK (activateHandlerST), this);
	g_application_run(G_APPLICATION (m_pApp), 0, 0);//argc, argv);
	g_object_unref(m_pApp);
	//gdk_threads_leave();
	m_pApp = 0;
	m_pMainWindow = 0;

	m_pParent->stopApplication(0);

	return NULL;
}

result_t CAccInterface::init()
{
	result_t	nresult;

	shell_assert(m_pApp == 0);

	nresult = m_thInterface.start(THREAD_CALLBACK(CAccInterface::thread, this));
	if ( nresult != ESUCCESS )  {
		log_error(L_GEN, "[acc_interface] failed to start thread, nresult: %d\n", nresult);
	}

	return nresult;
}

void CAccInterface::terminate()
{
	gdk_threads_enter();
	if ( m_pMainWindow ) {
		g_signal_emit_by_name(m_pMainWindow, "destroy");
	}
	gdk_threads_leave();
	m_thInterface.stop();
}
