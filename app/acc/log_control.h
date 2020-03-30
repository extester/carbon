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
 *  Revision 1.0, 16.09.2015 12:55:49
 *      Initial revision.
 */

#ifndef __LOG_CONTROL_H_INCLUDED__
#define __LOG_CONTROL_H_INCLUDED__

#include <gtk/gtk.h>

#include "shell/shell.h"

#define LOG_CONTROL_DISABLED				(-1)
#define LOG_CONTROL_LOCAL					0
#define LOG_CONTROL_CENTER					1
#define LOG_CONTROL_EXTERNAL				2

class CAccInterface;

class CLogControl
{
	private:
		CAccInterface*	m_pParent;
		GtkWidget**		m_arChannel;
		GtkWidget*		m_pLabel;
		boolean_t		m_bEnabled;
		boolean_t		m_bNotify;

	public:
		CLogControl(CAccInterface* pParent);
		~CLogControl();

	public:
		GtkWidget* create(const char* strTitle);
		void update(const uint8_t* pChannel);
		void clear();

		GtkWidget* getLabel() const { return m_pLabel; }
		void setLabel(const char* strLabel);

	private:
		void enable();

		static void onCheckST(GtkToggleButton* pCheck, gpointer user_data) {
			static_cast<CLogControl*>(user_data)->onCheck(pCheck);
		}
		void onCheck(GtkToggleButton* pCheck);
};


#endif /* __LOG_CONTROL_H_INCLUDED__ */