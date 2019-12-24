/*
 *  Carbon/DB module
 *  Database base class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 28.07.2015 23:15:47
 *      Initial revision.
 */

#ifndef __CARBON_DB_H_INCLUDED__
#define __CARBON_DB_H_INCLUDED__

#include "shell/object.h"

class CDb : public CObject
{
	public:
		CDb(const char* strName) : CObject(strName) {}
		virtual ~CDb() {}
};

#endif /* __CARBON_DB_H_INCLUDED__ */
