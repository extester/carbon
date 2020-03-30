/*
 *  Carbon framework
 *  XML configuration class
 *
 *  Copyright (c) 2015 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 25.09.2015 18:28:18
 *      Initial revision.
 */
/*
 * Config file structure:
 *
 *      <?xml version="1.0" encoding="UTF-8" standalone="no"?>
 *      <setting>
 *          <node> ... </node>
 *      </setting>
 */

#include "carbon/utils.h"
#include "carbon/memory.h"
#include "carbon/logger.h"
#include "carbon/config_xml.h"

#define CONFIG_VERSION_NAME         "version"
#define CONFIG_VERSION              "1.0"
#define CONFIG_ROOT_NAME            "setting"

/*
 * Create a new configuration object
 *
 *      bThreadSafe         TRUE: use locking for access object
 *                          FALSE: do not use thread-safe locking
 */
CConfig::CConfig(boolean_t bThreadSafe) :
    CObject("xml_config"),
    m_bModified(FALSE),
    m_bThreadSafe(bThreadSafe)
{
    *m_strFilename = '\0';
}

CConfig::~CConfig()
{
    update();
}

/*
 * Convert PUGI status code to the result_t
 *
 *      status      PUGI status code
 *
 * Return: result code
 */
result_t CConfig::parse_status2result(pugi::xml_parse_status status)
{
    result_t    nresult;

    switch ( status )  {
        case pugi::status_ok:                 nresult = ESUCCESS; break;
        case pugi::status_file_not_found:     nresult = ENOENT; break;
        case pugi::status_io_error:           nresult = EIO; break;
        case pugi::status_out_of_memory:      nresult = ENOMEM; break;
        case pugi::status_internal_error:     nresult = EFAULT; break;
        default:
            nresult = EINVAL; break;
    }

    return nresult;
}

/*
 * Create a empty XML config file
 *
 *      strFilename         config full filename
 *
 * Return: ESUCCESS, ENOMEM, ...
 *
 * Note: the object must be locked.
 */
result_t CConfig::doCreate(const char* strFilename)
{
    pugi::xml_node      root, node;
    pugi::xml_attribute attr;
    result_t            nresult;

    m_xmlDoc.reset();
    m_strFilename[0] = '\0';

    node = m_xmlDoc.append_child(pugi::node_declaration);
    if ( !node )  {
        return ENOMEM;
    }

    node.append_attribute("version") = "1.0";
    node.append_attribute("encoding") = "UTF-8";
    node.append_attribute("standalone") = "no";

    root = m_xmlDoc.append_child(CONFIG_ROOT_NAME);
    if ( root ) {
        attr = root.append_attribute(CONFIG_VERSION_NAME);
        if ( attr )  { attr.set_value(CONFIG_VERSION); }

        m_bModified = TRUE;
        copyString(m_strFilename, strFilename, sizeof(m_strFilename));
        nresult = ESUCCESS;
    }
    else {
        m_xmlDoc.reset();
        nresult = ENOMEM;
    }

    return nresult;
}

/*
 * Create a new empty config object
 *
 *      strFile         full filename
 *
 * Return: ESUCCESS, ...
 */
result_t CConfig::create(const char* strFilename)
{
    result_t    nresult;

    shell_assert(strFilename && *strFilename);

    lock();
    nresult = doCreate(strFilename);
    unlock();

    if ( nresult != ESUCCESS )  {
        log_error(L_GEN, "[config] failed to create config %s, result: %d\n",
                    strFilename, nresult);
    }

    return nresult;
}

/*
 * Load and parse a XML file
 *
 *      strFilename         full filename to parse
 *      bCreate             TRUE: create empty config if a file is not found
 *
 * Return: ESUCCESS, ENOENT, ...
 */
result_t CConfig::load(const char* strFilename, boolean_t bCreate)
{
    pugi::xml_parse_result  xml_result;
    result_t                nresult;

    lock();

    xml_result = m_xmlDoc.load_file(strFilename);
    if ( xml_result ) {
        /* File loaded and parsed */
        m_bModified = FALSE;
        copyString(m_strFilename, strFilename, sizeof(m_strFilename));
        nresult = ESUCCESS;
    }
    else {
        if ( xml_result.status == pugi::status_file_not_found && bCreate ) {
            /* File not found, create new one */
            nresult = doCreate(strFilename);
            if ( nresult != ESUCCESS )  {
                log_debug(L_GEN, "[config] failed to create config %s, result: %d\n",
                            strFilename, nresult);
            }
        }
        else {
            nresult = parse_status2result(xml_result.status);
            log_debug(L_GEN, "[config] failed to load config %s, result %d, parse status %d\n",
                      strFilename, nresult, xml_result.status);
        }
    }

    unlock();

    return nresult;
}

/*
 * Save a loaded/new configuration to the file
 *
 * Return: ESUCCESS, EIO, ...
 *
 * Note: filename must be previously set by load()/setFile().
 */
result_t CConfig::save()
{
    result_t    nresult = ESUCCESS;
    boolean_t   bResult;

    lock();

    shell_assert(m_strFilename[0] != '\0');
    if ( m_strFilename[0] != '\0' )  {
        bResult = m_xmlDoc.save_file(m_strFilename, "\t", pugi::format_default, pugi::encoding_utf8);
        if ( bResult )  {
            m_bModified = FALSE;
        }
        else {
            log_debug(L_GEN, "[config] failed to save file %s\n", m_strFilename);
            nresult = EIO;
        }
    }

    unlock();

    return nresult;
}

/*
 * Save a setting if they are modified
 *
 * Return: ESUCCESS, EIO, ...
 */
result_t CConfig::update()
{
    boolean_t   bModified;
    result_t    nresult = ESUCCESS;

    lock();
    bModified = m_bModified;
    unlock();

    if ( bModified )  {
        nresult = save();
    }

    return nresult;
}

/*
 * Read a single string parameter
 *
 *      strPath         parameter path, i.e "main_window/top"
 *      strBuffer       output buffer
 *      size            outpur buffer size, bytes
 *
 * Return: ESUCCESS, ENOENT, ENOMEM, ...
 */
result_t CConfig::get(const char* strPath, char* strBuffer, size_t size)
{
    char*               strTmp;
    pugi::xpath_node    xnode;
    result_t            nresult = ENOENT;

    shell_assert(*strPath != '\0' && *strPath != '/');
    shell_assert(strBuffer);
    shell_assert(size);

    strTmp = (char*)memAlloca(_tstrlen(strPath)+sizeof(CONFIG_ROOT_NAME)+16);
    _tstrcpy(strTmp, CONFIG_ROOT_NAME);
    _tstrcat(strTmp, "/");
    _tstrcat(strTmp, strPath);

    lock();

    xnode = m_xmlDoc.select_node(strTmp);
    if ( xnode )  {
        copyString(strBuffer, xnode.node().first_child().value(), size);
        nresult = ESUCCESS;
    }

    unlock();

    return nresult;
}

/*
 * Read a single numeric parameter
 *
 *      strPath         parameter path, i.e "main_window/top"
 *      pnValue         output value [out]
 *
 * Return: ESUCCESS, ENOENT, ENOMEM, ...
 */
result_t CConfig::get(const char* strPath, int64_t* pnValue)
{
    char        strTmp[64];
    result_t    nresult;

    nresult = get(strPath, strTmp, sizeof(strTmp));
    if ( nresult == ESUCCESS )  {
        int64_t     value;
        char*       e;

        value = (int64_t)_tstrtoll(strTmp, &e, 10);
        if ( *e == '\0' )  {
            *pnValue = value;
        }
        else {
            log_debug(L_GEN, "[config] parameter '%s' is not a number '%s'\n", strPath, strTmp);
            nresult = EINVAL;
        }
    }

    return nresult;
}

/*
 * Write a single string parameter
 *
 *      strPath         parameter path, i.e "main_window/top"
 *      strValue        parameter value
 *
 * Return: ESUCCESS, EINVAL, ...
 */
result_t CConfig::set(const char* strPath, const char* strValue)
{
    pugi::xml_node  root, node;
    const char      *s, *e;
    result_t        nresult;

    shell_assert(*strPath != '\0' && *strPath != '/');

    lock();

    root = m_xmlDoc.child(CONFIG_ROOT_NAME);
    if ( !root )  {
        root = m_xmlDoc.append_child(CONFIG_ROOT_NAME);
        if ( root )  {
            m_bModified = TRUE;
        }
    }

    if ( !root )  {
        unlock();
        log_debug(L_GEN, "[config] failed to append root node\n");
        return EINVAL;
    }

    nresult = ESUCCESS;
    s = strPath;
    node = root;

    while( *s && nresult == ESUCCESS )  {
        char    strElem[64];

        e = _tstrchr(s, '/');
        if ( e ) {
            size_t  l = sh_min(sizeof(strElem)-1, A(e)-A(s));

            UNALIGNED_MEMCPY(strElem, s, l);
            strElem[l] = '\0';
            s = e+1;
        }
        else {
            copyString(strElem, s, sizeof(strElem));
            s += _tstrlen(s);
        }

        nresult = ENOENT;
        for(pugi::xml_node_iterator it = node.begin(); it != node.end(); it++) {
            if ( _tstrcmp(it->name(), strElem) == 0 ) {
                if ( e )  {
                    /* Intermediate node */
                    node = *it;
                    nresult = ESUCCESS;
                }
                else {
                    /* Value node */
                    if ( it->first_child().set_value(strValue)) {
                        nresult = ESUCCESS;
                        m_bModified = TRUE;
                    }
                    else {
                        log_debug(L_GEN, "[config] failed to set node '%s'\n", strElem);
                        nresult = EFAULT;
                    }
                }
                break;
            }
        }

        if ( e )   {
            if ( nresult == ENOENT )  {
                /* Intemediate node doesn't exist */
                node = node.append_child(strElem);
                if ( node ) {
                    nresult = ESUCCESS;
                    m_bModified = TRUE;
                }
                else {
                    log_debug(L_GEN, "[config] failed to insert intermedialte node '%' of "
                              "full node '%s'\n", strElem, strPath);
                    nresult = EFAULT;
                }
            }
            continue;
        }

        if ( nresult == ENOENT ) {
            node = node.append_child(strElem);
            if ( node ) {
                node = node.append_child(pugi::node_pcdata);
                if ( node ) {
                    if ( node.set_value(strValue)) {
                        m_bModified = TRUE;
                        nresult = ESUCCESS;
                    }
                    else {
                        log_debug(L_GEN, "[config] failed to append node '%s' value '%s'\n",
                                  strElem, strValue);
                        nresult = EFAULT;
                    }
                }
                else {
                    log_debug(L_GEN,"[config] failed to append node '%s' value\n",
                              strPath);
                    nresult = EFAULT;
                }
            }
            else {
                log_debug(L_GEN, "[config] failed to append node '%s'\n", strPath);
                nresult = EFAULT;
            }
        }
    }

    unlock();

    return nresult;
}

/*
 * Write a single string parameter
 *
 *      strPath         parameter path, i.e "main_window/top"
 *      nValue          value
 *
 * Return: ESUCCESS, EINVAL, ...
 */
result_t CConfig::set(const char* strPath, int64_t nValue)
{
    char        strTmp[32];
    result_t    nresult;

    _tsnprintf(strTmp, sizeof(strTmp), "%lld", (signed long long int)nValue);
    nresult = set(strPath, strTmp);

    return nresult;
}

