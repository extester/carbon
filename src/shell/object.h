/*
 *  Shell library
 *  Global object class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 30.03.2015 14:50:41
 *      Initial revision.
 *
 *  Revision 1.1, 22.05.2017 14:34:46
 *  	Added setName() member.
 */

#ifndef __SHELL_OBJECT_H_INCLUDED__
#define __SHELL_OBJECT_H_INCLUDED__

#include "shell/config.h"
#include "shell/shell.h"

class CObject
{
	private:
		char		m_strName[CARBON_OBJECT_NAME_LENGTH];

	protected:
		CObject(const char* strName = "")
		{
			setName(strName);
		}

		virtual ~CObject()
		{
		}

	public:
		virtual const char* getName() const { return m_strName; }
		virtual void setName(const char* strName) {
			if ( strName ) {
				copyString(m_strName, strName, sizeof(m_strName));
			}
		}
};

#endif /* __SHELL_OBJECT_H_INCLUDED__ */

