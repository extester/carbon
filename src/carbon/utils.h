/*
 *  Carbon framework
 *  Utility functions
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.01.2017 14:19:20
 *      Initial revision.
 */

#ifndef __CARBON_UTILS_H_INCLUDED__
#define __CARBON_UTILS_H_INCLUDED__

#include <vector>

#include "shell/utils.h"

#include "carbon/cstring.h"

typedef std::vector<CString>		str_vector_t;

extern size_t strSplit(const char* strData, char chSep, str_vector_t* strAr, const char* strTrim = 0);

extern boolean_t strParseSemicolonKeyValue(const char* strData, const char* strKey,
										   CString& strValue, const char* strTrim = 0);

typedef struct {
	CString			scheme;     	/* mandatory */
	CString			host;			/* mandatory */
	int				port;       	/* optional */
	CString			path;       	/* optional */
	CString			query;			/* optional */
	CString			fragment;		/* optional */
	CString			username;		/* optional */
	CString			password;		/* optional */
} split_url_t;

extern boolean_t splitUrl(const char* strUrl, split_url_t* pData);

extern CString getDirname(const char* strPath);

#endif /* __CARBON_UTILS_H_INCLUDED__ */
