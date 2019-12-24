/*
 *  Carbon framework
 *  XML configuration
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.09.2015 18:13:06
 *      Initial revision.
 */

#ifndef __CARBON_CONFIG_XML_H_INCLUDED__
#define __CARBON_CONFIG_XML_H_INCLUDED__

#include <pugixml/pugixml.hpp>

#include "shell/shell.h"
#include "shell/object.h"

#include "carbon/lock.h"

class CConfig : public CObject
{
    protected:
        pugi::xml_document      m_xmlDoc;
        char                    m_strFilename[128];
        boolean_t               m_bModified;

    private:
        boolean_t               m_bThreadSafe;
        CMutex                  m_lock;

    public:
        CConfig(boolean_t bThreadSafe = TRUE);
        virtual ~CConfig();

    public:
        virtual result_t create(const char* strFilename);
        virtual result_t load(const char* strFilename, boolean_t bCreate = FALSE);
        virtual result_t save();
        virtual result_t update();

        virtual result_t get(const char* strPath, char* strBuffer, size_t size);
        virtual result_t get(const char* strPath, int64_t* pnValue);
        virtual result_t get(const char* strPath, int* pnValue)  {
            int64_t     tmp;
            result_t    nresult;

            nresult = get(strPath, &tmp);
            if ( nresult == ESUCCESS )  {
                *pnValue = (int)tmp;
            }
            return nresult;
        }
        virtual result_t get(const char* strPath, int64_t* pnValue, int64_t nDefault) {
            result_t    nresult;

            nresult = get(strPath, pnValue);
            if ( nresult != ESUCCESS )  {
                *pnValue = nDefault;
            }
            return nresult;
        }
        virtual result_t get(const char* strPath, int* pnValue, int nDefault) {
            result_t    nresult;

            nresult = get(strPath, pnValue);
            if ( nresult != ESUCCESS )  {
                *pnValue = nDefault;
            }
            return nresult;
        }

        virtual result_t set(const char* strPath, const char* strValue);
        virtual result_t set(const char* strPath, int64_t nValue);
        virtual result_t set(const char* strPath, int nValue)  {
            return set(strPath, (int64_t)nValue);
        }

    protected:
        void lock() {
            if ( m_bThreadSafe )  {
                m_lock.lock();
            }
        }

        void unlock()  {
            if ( m_bThreadSafe )  {
                m_lock.unlock();
            }
        }

    private:
        result_t doCreate(const char* strFilename);
        result_t parse_status2result(pugi::xml_parse_status status);
};

#endif /* __CARBON_CONFIG_XML_H_INCLUDED__ */
