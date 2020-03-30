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
 *  Revision 1.0, 14.07.2015 16:27:32
 *      Initial revision.
 */

#ifndef __INTERFACE_H_INCLUDED__
#define __INTERFACE_H_INCLUDED__

#include <vector>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "carbon/thread.h"
#include "shell/netaddr.h"
#include "shell/hr_time.h"
#include "contact/icmp.h"
#include "shell/ref_object.h"

#include "carbon/module.h"

#include "memory_stat.h"
#include "netconn_stat.h"
#include "log_control.h"
#include "../center/packet_center.h"

typedef struct host_item
{
	host_id_t		id;
	CNetHost		ipaddr;
	char 			strName[32];
	hr_time_t		hrPing;
} host_item_t;


class CAccApp;

class CAccInterface : public CModule
{
	private:
		CAccApp*			m_pParent;
		CThread				m_thInterface;

		GtkApplication*		m_pApp;
		GtkWidget*			m_pMainWindow;
		GtkWidget*			m_pOnOff;
		GtkToolItem*		m_pUpdateButton;
		GtkToolItem*		m_pDeleteButton;
		GtkWidget*			m_pListView;
		GtkWidget*			m_pNetState;
		GtkWidget*			m_pBusyState;
		GdkPixbuf*			m_arNetState[2];
		GtkWidget*			m_pCenterVersion;

		GtkWidget*			m_pMemStatDialog;
		CMemoryStat*		m_arMemStat[3];
		int 				m_nMemStatType;
		CNetHost			m_memStatExtAddr;

		GtkWidget*			m_pNetConnStatDialog;
		CNetConnStat*		m_arNetConnStat[3];
		int					m_nNetConnStatType;
		CNetHost			m_netconnStatExtAddr;

		GtkWidget*			m_pLogCtlDialog;
		CLogControl*		m_arLogCtl[3];
		int					m_nLogCtlType;
		CNetHost			m_logctlExtAddr;

		std::vector<host_item_t>	m_arHost;

	public:
		CAccInterface(CAccApp* pParent);
		virtual ~CAccInterface();

	public:
		virtual result_t init();
		virtual void terminate();

		virtual void dump(const char* strPref = "") const  { shell_unused(strPref); }

		void updateHost(host_id_t id, const char* strName, ip_addr_t ipAddr, hr_time_t hrPing);
		void updateNetState(boolean_t bOnline);
		void updateCenterVersion(const char* strVersion);
		void updateBusyState(boolean_t bShow);
		void updateMemoryStat(int type, const memory_stat_t* pStat);
		void updateNetConnStat(int type, const tcpconn_stat_t* pStat);
		void updateLogControl(int type, const uint8_t* pChannel);
		void onLogControlCheck(unsigned int nChannel, boolean_t isOn);

	private:
		host_item_t* createHost(host_id_t id, const char* strName,
								ip_addr_t ipAddr, hr_time_t hrPing);
		void addHost(host_id_t id, const char* strName, ip_addr_t ipAddr, hr_time_t hrPing);
		void addHostRow(int index);
		void deleteHost(int index);
		void formatValueMs(hr_time_t hrTime, char* strBuf, size_t length);

		int findHost(host_id_t id) const;
		void updateHostTime(int index);
		boolean_t doHostDialog(const char* strTitle, const char* strButton, char* strHost, char* strIp);
		void doMemoryStat(int type, const CNetHost& extAddr);
		void doNetConnStat(int type, const CNetHost& extAddr);
		void doLogControl(int type, const CNetHost& extAddr);

		void* thread(CThread* pThread, void* pData);

		static void activateHandlerST(GtkApplication* app, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->activateHandler();
		}
		void activateHandler();

		static void onConfigureMainWindowST(GtkWindow* window, GdkEvent* event, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onConfigureMainWindow(window, event);
		}
		void onConfigureMainWindow(GtkWindow* pWindow, GdkEvent* pEvent);

		static boolean_t onOffHandlerST(GtkWidget* wdt, void* unknown, gpointer user_data) {
			return static_cast<CAccInterface*>(user_data)->onOffHandler();
		}
		boolean_t onOffHandler();

		static void onNewHostST(GtkToolButton* widget, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onNewHost(widget);
		}
		void onNewHost(GtkToolButton* widget);

		static void onUpdateHostST(GtkToolButton* widget, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onUpdateHost(widget);
		}
		void onUpdateHost(GtkToolButton* widget);

		static void onDeleteHostST(GtkToolButton* widget, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onDeleteHost(widget);
		}
		void onDeleteHost(GtkToolButton* widget);

		static void onQuitST(GtkToolButton* widget, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onQuit(widget);
		}
		void onQuit(GtkToolButton* widget);

		static void onSelectionST(GtkTreeSelection* pSelection, gpointer user_data)  {
			static_cast<CAccInterface*>(user_data)->onSelection(pSelection);
		}
		void onSelection(GtkTreeSelection* pSelection);

		/* Memory Dialog */
		static void onMemoryStatDialogST(GtkToolButton* widget, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onMemoryStatDialog(widget);
		}
		void onMemoryStatDialog(GtkToolButton* widget);

		static void onMemoryStatDialogResponseST(GtkDialog* pDialog, gint response_id, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onMemoryStatDialogResponse(pDialog, response_id);
		}
		void onMemoryStatDialogResponse(GtkDialog* pDialog, int responseId);

		static void onMemoryStatPageST(GtkNotebook* pNotebook, GtkWidget* pPage,
											guint page_num, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onMemoryStatPage(page_num);
		}
		void onMemoryStatPage(int nPage);

		boolean_t runDialogExtAddress(GtkDialog* pParent, CNetHost& extAddr);

		/* Network connector dialog */
		static void onNetConnStatDialogST(GtkToolButton* widget, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onNetConnStatDialog(widget);
		}
		void onNetConnStatDialog(GtkToolButton* widget);

		static void onNetConnStatDialogResponseST(GtkDialog* pDialog, gint response_id, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onNetConnStatDialogResponse(pDialog, response_id);
		}
		void onNetConnStatDialogResponse(GtkDialog* pDialog, int responseId);

		static void onNetConnStatPageST(GtkNotebook* pNotebook, GtkWidget* pPage, guint page_num, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onNetConnStatPage(page_num);
		}
		void onNetConnStatPage(int nPage);

		/* Log Control dialog */
		static void onLogControlDialogST(GtkToolButton* widget, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onLogControlDialog(widget);
		}
		void onLogControlDialog(GtkToolButton* widget);

		static void onLogControlDialogResponseST(GtkDialog* pDialog, gint response_id, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onLogControlDialogResponse(pDialog, response_id);
		}
		void onLogControlDialogResponse(GtkDialog* pDialog, int responseId);

		static void onLogControlPageST(GtkNotebook* pNotebook, GtkWidget* pPage, guint page_num, gpointer user_data) {
			static_cast<CAccInterface*>(user_data)->onLogControlPage(page_num);
		}
		void onLogControlPage(int nPage);
};

#endif /* __INTERFACE_H_INCLUDED__ */