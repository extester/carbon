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
 *  Revision 1.0, 11.01.2017 14:19:42
 *      Initial revision.
 */

#include "shell/shell.h"
#include "shell/file.h"

#include "carbon/utils.h"

/*
 * Split string by character
 *
 *      strData         string to split
 *      chSep           separator character
 *      strAr           output data array
 *      strTrim         trim splitted string by the characters
 *
 * Return: count of output array elements
 *
 * Format:
 *      <string><sep><string><sep><string><sep><string><sep>
 */
size_t strSplit(const char* strData, char chSep, str_vector_t* strAr, const char* strTrim)
{
    const char	*s = strData, *p;

    strAr->clear();
    if ( strData )  {
        while ( *s != '\0' )  {
            p = _tstrchr(s, chSep);
            if ( p )  {
                CString		str(s, A(p)-A(s));

                if ( strTrim )  {
                    str.trim(strTrim);
                }

                strAr->push_back(str);
                s = p+1;
            }
            else {
                CString		str(s);

                if ( strTrim )  {
                    str.trim(strTrim);
                }

                strAr->push_back(str);
                s += _tstrlen(s);
            }
        }
    }

    return strAr->size();
}

/*
 * Parse a value of the key
 *
 * 		strData			input string to parse
 * 		strKey			key to find
 * 		strValue		value [out]
 *
 * Return: TRUE - success, FALSE - was not found
 *
 * Format:
 *
 * 		key=value;key=value;key=value;....
 */
boolean_t strParseSemicolonKeyValue(const char* strData, const char* strKey,
                                    CString& strValue, const char* strTrim)
{
    str_vector_t	strAr;
    size_t			size, i, keyLen = _tstrlen(strKey);
    const char*		s;
    boolean_t		bResult = FALSE;

    if ( strData )  {
        size = strSplit(strData, ';', &strAr, strTrim);
        for(i=0; i<size; i++)  {
            if ( strTrim ) {
                strAr[i].trim(strTrim);
            }

            s = _tstrchr(strAr[i], '=');
            if ( (A(s)-A((const char*)strAr[i])) == keyLen &&
                 _tmemcmp(strKey, (const char*)strAr[i], keyLen) == 0 )
            {
                strValue = ((const char*)strAr[i]) + keyLen + 1;
                bResult = TRUE;
                break;
            }
        }
    }

    return bResult;
}

static __inline__ int _is_scheme_char(int c)
{
    return ( (c>='a'&&c<='z') || c== '+' || c== '-' || c=='.' );
    //return (!isalpha(c) && '+' != c && '-' != c && '.' != c) ? 0 : 1;
}

/*
 * Split URL to the components
 *
 *      strUrl          URL to split
 *      pData           output components
 *
 * Return:
 *      TRUE            success
 *      FALSE           can't split, invalid URL format
 */
boolean_t splitUrl(const char* strUrl, split_url_t* pData)
{
    const char *tmpstr;
    const char *curstr;
    int len;
    int i;
    int userpass_flag;
    int bracket_flag;

    curstr = strUrl;

    /*
	 * <scheme>:<scheme-specific-part>
	 * <scheme> := [a-z\+\-\.]+
	 *             upper case = lower case for resiliency
	 */
    /* Read scheme */
    tmpstr = _tstrchr(curstr, ':');
    if ( NULL == tmpstr ) {
        /* Not found the character */
        return FALSE;
    }

    /* Get the scheme length */
    len = tmpstr - curstr;
    /* Check restrictions */
    for ( i = 0; i < len; i++ ) {
        if ( !_is_scheme_char(curstr[i]) ) {
            /* Invalid format */
            return FALSE;
        }
    }

    /* Copy the scheme to the storage */
    pData->scheme = CString(curstr, len);

    /* Skip ':' */
    tmpstr++;
    curstr = tmpstr;

    /*
	 * //<user>:<password>@<host>:<port>/<url-path>
	 * Any ":", "@" and "/" must be encoded.
	 */
    /* Eat "//" */
    for ( i = 0; i < 2; i++ ) {
        if ( '/' != *curstr ) {
            return FALSE;
        }
        curstr++;
    }

    /* Check if the user (and password) are specified. */
    userpass_flag = 0;
    tmpstr = curstr;
    while ( '\0' != *tmpstr ) {
        if ( '@' == *tmpstr ) {
            /* Username and password are specified */
            userpass_flag = 1;
            break;
        } else if ( '/' == *tmpstr ) {
            /* End of <host>:<port> specification */
            userpass_flag = 0;
            break;
        }
        tmpstr++;
    }

    /* User and password specification */
    tmpstr = curstr;
    if ( userpass_flag ) {
        /* Read username */
        while ( '\0' != *tmpstr && ':' != *tmpstr && '@' != *tmpstr ) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        pData->username = CString(curstr, len);

        /* Proceed current pointer */
        curstr = tmpstr;
        if ( ':' == *curstr ) {
            /* Skip ':' */
            curstr++;
            /* Read password */
            tmpstr = curstr;
            while ( '\0' != *tmpstr && '@' != *tmpstr ) {
                tmpstr++;
            }
            len = tmpstr - curstr;
            pData->password = CString(curstr, len);
            curstr = tmpstr;
        }
        /* Skip '@' */
        if ( '@' != *curstr ) {
            return FALSE;
        }
        curstr++;
    }

    if ( '[' == *curstr ) {
        bracket_flag = 1;
    } else {
        bracket_flag = 0;
    }
    /* Proceed on by delimiters with reading host */
    tmpstr = curstr;
    while ( '\0' != *tmpstr ) {
        if ( bracket_flag && ']' == *tmpstr ) {
            /* End of IPv6 address. */
            tmpstr++;
            break;
        } else if ( !bracket_flag && (':' == *tmpstr || '/' == *tmpstr) ) {
            /* Port number is specified. */
            break;
        }
        tmpstr++;
    }
    len = tmpstr - curstr;
    pData->host = CString(curstr, len);
    curstr = tmpstr;

    /* Is port number specified? */
    if ( ':' == *curstr ) {
        CString		strPort;

        curstr++;
        /* Read port number */
        tmpstr = curstr;
        while ( '\0' != *tmpstr && '/' != *tmpstr ) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        strPort = CString(curstr, len);
        curstr = tmpstr;

        if ( strPort.getNumber(pData->port) != ESUCCESS ) {
            return FALSE;
        }
    }

    /* End of the string */
    if ( '\0' == *curstr ) {
        return TRUE;
    }

    /* Skip '/' */
    if ( '/' != *curstr ) {
        return FALSE;
    }
    curstr++;

    /* Parse path */
    tmpstr = curstr;
    while ( '\0' != *tmpstr && '#' != *tmpstr  && '?' != *tmpstr ) {
        tmpstr++;
    }
    len = tmpstr - curstr;
    pData->path = CString(curstr, len);
    curstr = tmpstr;

    /* Is query specified? */
    if ( '?' == *curstr ) {
        /* Skip '?' */
        curstr++;
        /* Read query */
        tmpstr = curstr;
        while ( '\0' != *tmpstr && '#' != *tmpstr ) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        pData->query = CString(curstr, len);
        curstr = tmpstr;
    }

    /* Is fragment specified? */
    if ( '#' == *curstr ) {
        /* Skip '#' */
        curstr++;
        /* Read fragment */
        tmpstr = curstr;
        while ( '\0' != *tmpstr ) {
            tmpstr++;
        }
        len = tmpstr - curstr;
        pData->fragment = CString(curstr, len);
        curstr = tmpstr;
    }

    return TRUE;
}

/*
 * Get directory name from path.
 * Return empty string if a path is empty or no path separator wes found
 *
 *      strPath         source path
 */
CString getDirname(const char* strPath)
{
    CString         strDir;
    const char*     s;

    if ( strPath != 0 && *strPath != '\0' )  {
        s = _tstrrchr(strPath, PATH_SEPARATOR);
        if ( s != 0 )  {
            if ( s != strPath )  {
                strDir = CString(strPath, A(s)-A(strPath));
            }
            else {
                char t[10];

                _tsnprintf(t, sizeof(t), "%c", PATH_SEPARATOR);
                strDir = t;
            }
        }
    }

    return strDir;
}
