/*
 *  Carbon Framework Network Client
 *  Memory Statistic page
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 13.09.2015-13 17:02:49
 *      Initial revision.
 */

#ifndef __MEMORY_STAT_H_INCLUDED__
#define __MEMORY_STAT_H_INCLUDED__

#include <gtk/gtk.h>

#include "shell/hr_time.h"

#include "carbon/memory.h"

class CAccInterface;

#define MEMORY_STAT_DISABLED				(-1)
#define MEMORY_STAT_LOCAL					0
#define MEMORY_STAT_CENTER					1
#define MEMORY_STAT_EXTERNAL				2

#define MEMORY_STAT_INTERVAL				HR_2SEC

class CMemoryStat
{
	private:
		GtkWidget*		m_pFullAllocCount;
		GtkWidget*		m_pFreeCount;
		GtkWidget*		m_pAllocCount;
		GtkWidget*		m_pAllocSize;
		GtkWidget*		m_pAllocSizeMax;
		GtkWidget*		m_pReallocCount;
		GtkWidget*		m_pFailCount;

		GtkWidget*		m_pLabel;

	public:
		CMemoryStat();
		~CMemoryStat();

	public:
		GtkWidget* create(const char* strTitle);
		void update(const memory_stat_t* pStat);
		void clear();

		GtkWidget* getLabel() const { return m_pLabel; }
		void setLabel(const char* strLabel);

	private:
		void formatSize(size_t size, char* strBuffer, size_t length) const;
};


#endif /* __MEMORY_STAT_H_INCLUDED__ */