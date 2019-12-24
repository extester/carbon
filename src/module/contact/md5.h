/*
 *  Carbon/Contact module
 *  MD5 Hash calculation
 *
 *  Copyright (c) 2017 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 14.11.2017 15:03:30
 *      Initial revision.
 */

#ifndef __CONTACT_MD5_H_INCLUDED__
#define __CONTACT_MD5_H_INCLUDED__

#include "shell/shell.h"
#include "contact/md5lib.h"

class CMd5
{
	public:
		CMd5() {}
		~CMd5() {}

	public:
		void md5(const void* pData, size_t length, char* strMd5);
		void md5(const char* strData, char* strMd5) {
			return md5(strData, _tstrlen(strData), strMd5);
		}
};

#endif /* __CONTACT_MD5_H_INCLUDED__ */
