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

#include <gtk/gtk.h>

#include "memory_stat.h"

CMemoryStat::CMemoryStat() :
	m_pFullAllocCount(0),
	m_pFreeCount(0),
	m_pAllocCount(0),
	m_pAllocSize(0),
	m_pAllocSizeMax(0),
	m_pReallocCount(0),
	m_pFailCount(0),
	m_pLabel(0)
{
}

CMemoryStat::~CMemoryStat()
{
}

void CMemoryStat::clear()
{
	if ( m_pFullAllocCount )  {
		gtk_label_set_text(GTK_LABEL(m_pFullAllocCount), "");
	}
	if ( m_pFreeCount )  {
		gtk_label_set_text(GTK_LABEL(m_pFreeCount), "");
	}
	if ( m_pAllocCount )  {
		gtk_label_set_text(GTK_LABEL(m_pAllocCount), "");
	}
	if ( m_pAllocSize )  {
		gtk_label_set_text(GTK_LABEL(m_pAllocSize), "");
	}
	if ( m_pAllocSizeMax )  {
		gtk_label_set_text(GTK_LABEL(m_pAllocSizeMax), "");
	}
	if ( m_pReallocCount )  {
		gtk_label_set_text(GTK_LABEL(m_pReallocCount), "");
	}
	if ( m_pFailCount )  {
		gtk_label_set_text(GTK_LABEL(m_pFailCount), "");
	}
}

GtkWidget* CMemoryStat::create(const char* strTitle)
{
	GtkWidget	*pGrid, *pWidget;
	size_t		i, count;
	struct { const char* strName; GtkWidget** pp; } items[] = {
		{	"Full alloc count:",	&m_pFullAllocCount	},
		{	"Free count:",			&m_pFreeCount		},
		{	"Alloc count:",			&m_pAllocCount		},
		{	"Alloc size:",			&m_pAllocSize		},
		{	"Maximum alloc size:",	&m_pAllocSizeMax	},
		{	"Realloc count:",		&m_pReallocCount	},
		{	"Fail count:",			&m_pFailCount		}
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
		gtk_grid_attach(GTK_GRID(pGrid), pWidget, 0, (gint)i, 1, 1);

		*items[i].pp = gtk_label_new("");
		g_object_set(*items[i].pp, "xalign", 0.0, NULL);
		gtk_widget_set_size_request(*items[i].pp, 240, -1);
		gtk_grid_attach(GTK_GRID(pGrid), *items[i].pp, 1, (gint)i, 1, 1);
	}

	m_pLabel = gtk_label_new(strTitle);
	return pGrid;
}

void CMemoryStat::formatSize(size_t size, char* strBuffer, size_t length) const
{
	if ( size < 1024 )  {
		snprintf(strBuffer, length, "%lu", size);
		return;
	}

	if ( size < (16*1024) )  {
		if ( (size&0x3ff) != 0 ) {
			snprintf(strBuffer, length, "%luK %lub", size/1024, size&0x3ff);
		}
		else {
			snprintf(strBuffer, length, "%luK", size/1024);
		}
		return;
	}

	if ( size < (1024*1024) )  {
		snprintf(strBuffer, length, "%luK", size/1024);
		return;
	}

	size = size>>10;
	snprintf(strBuffer, length, "%luM %luK", size>>10, size&0x3ff);
}

void CMemoryStat::update(const memory_stat_t* pStat)
{
	char	strTmp[64];

	shell_assert(m_pLabel != 0);
	if ( m_pLabel == 0 )  {
		return;
	}

	snprintf(strTmp, sizeof(strTmp), "%d", atomic_get(&pStat->full_alloc_count));
	gtk_label_set_text(GTK_LABEL(m_pFullAllocCount), strTmp);

	snprintf(strTmp, sizeof(strTmp), "%d", atomic_get(&pStat->free_count));
	gtk_label_set_text(GTK_LABEL(m_pFreeCount), strTmp);

	snprintf(strTmp, sizeof(strTmp), "%d", atomic_get(&pStat->alloc_count));
	gtk_label_set_text(GTK_LABEL(m_pAllocCount), strTmp);

	formatSize((size_t)atomic_get(&pStat->alloc_size), strTmp, sizeof(strTmp));
	gtk_label_set_text(GTK_LABEL(m_pAllocSize), strTmp);

	formatSize((size_t)atomic_get(&pStat->alloc_size_max), strTmp, sizeof(strTmp));
	gtk_label_set_text(GTK_LABEL(m_pAllocSizeMax), strTmp);

	snprintf(strTmp, sizeof(strTmp), "%d", atomic_get(&pStat->realloc_count));
	gtk_label_set_text(GTK_LABEL(m_pReallocCount), strTmp);

	snprintf(strTmp, sizeof(strTmp), "%d", atomic_get(&pStat->fail_count));
	gtk_label_set_text(GTK_LABEL(m_pFailCount), strTmp);
}

void CMemoryStat::setLabel(const char* strLabel)
{
	gtk_label_set_text(GTK_LABEL(m_pLabel), strLabel);
}
