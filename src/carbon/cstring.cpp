/*
 *  Carbon framework
 *  String class object
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 01.07.2016 10:42:09
 *      Initial revision.
 *
 * 	Revision 1.1, 14.11.2017 15:58:25
 * 		Added subString(), insert().
 */

#include <new>

#include "shell/shell.h"

#include "carbon/utils.h"
#include "carbon/cstring.h"

#define STRING_BUFFER_STEP		64


/*******************************************************************************
 * Class CString
 */

CString::CString(const char* strData) :
	m_strData(0),
	m_nLength(0),
	m_nMaxLength(0)
{
	if ( strData )  {
		copyString(strData);
	}
}

CString::CString(const void* memData, size_t size) :
	m_strData(0),
	m_nLength(0),
	m_nMaxLength(0)
{
	append(memData, size);
}

CString::CString(const CString& str) :
	m_strData(0),
	m_nLength(0),
	m_nMaxLength(0)
{
	copyString(str);
}

CString::~CString()
{
	SAFE_FREE(m_strData);
}

/*
 * Copy a string to the object
 *
 * 		strData		string to copy (may be 0)
 */
void CString::copyString(const char* strData)
{
	clear();

	if ( strData ) {
		size_t len = _tstrlen(strData);

		makeBuffer(len+1);
		_tstrcpy(m_strData, strData);
		m_nLength = len;
	}
	else {
		makeBuffer(STRING_BUFFER_STEP);
	}
}

/*
 * Append memory to the object
 *
 * 		memData			memory data pointer
 * 		size			data length, bytes
 */
void CString::append(const void* memData, size_t size)
{
	if ( size > 0 ) {
		makeBuffer(m_nLength+1+size);
		UNALIGNED_MEMCPY(&m_strData[m_nLength], memData, size);
		m_strData[m_nLength+size] = '\0';
		m_nLength = _tstrlen(m_strData);
	}
}

/*
 * Append a string to the current string
 *
 * 		strData		string to append
 */
void CString::append(const char* strData)
{
	if ( strData ) {
		append(strData, _tstrlen(strData));
	}
}

void CString::appendPath(const char* strPath)
{
	size_t	size = _tstrlen(strPath);

	if ( size > 0 )  {
		makeBuffer(m_nLength+1+size+1+1);
		::appendPath(m_strData, strPath, m_nMaxLength);
		m_nLength = _tstrlen(m_strData);
	}
}

/*
 * Compare strings case sensitive
 *
 *		str			string to compare with the object's string
 *
 * Return: TRUE if strings are equal, FALSE otherwise
 */
boolean_t CString::equal(const CString* str) const
{
	return equal((const char*)str);
}

/*
 * Compare strings case sensitive
 *
 *		str			string to compare with the object's string
 *
 * Return: TRUE if strings are equal, FALSE otherwise
 */
boolean_t CString::equal(const char* strData) const
{
	size_t	len;

	if ( m_nLength == 0 && (strData == NULL || *strData == '\0') )  {
		return TRUE;
	}

	len = _tstrlen(strData);
	if ( m_nLength != len )  {
		return FALSE;
	}

	return _tstrcmp(m_strData, strData) == 0;
}

/*
 * Compare strings case insensitive
 *
 *		str			string to compare with the object's string
 *
 * Return: TRUE if strings are equal, FALSE otherwise
 */
boolean_t CString::equalNoCase(const CString* str) const
{
	return equalNoCase((const char*)str);
}

/*
 * Compare strings case insensitive
 *
 *		str			string to compare with the object's string
 *
 * Return: TRUE if strings are equal, FALSE otherwise
 */
boolean_t CString::equalNoCase(const char* strData) const
{
	size_t	len;

	if ( m_nLength == 0 && (strData == NULL || *strData == '\0') )  {
		return TRUE;
	}

	len = _tstrlen(strData);
	if ( m_nLength != len )  {
		return FALSE;
	}

	return _tstrcasecmp(m_strData, strData) == 0;
}

void CString::format(const char* strFormat, ...)
{
	size_t		l, len;
	char		*strBuf, *strBuf2;
	va_list		args;

	l = _tstrlen(strFormat);
	l = ALIGN(l, 128);
	strBuf = (char*)memAlloc(l);

	while ( TRUE ) {
		va_start(args, strFormat);
		len = _tvsnprintf(strBuf, l, strFormat, args);
		va_end(args);

		if ( len < (l-1) )  {
			break;
		}

		l = l*2;
		strBuf2 = (char*)memRealloc(strBuf, l);
		if ( !strBuf2 )  {
			memFree(strBuf);
			throw std::bad_alloc();
		}

		strBuf = strBuf2;
	}

	copyString(strBuf);
	memFree(strBuf);
}

result_t CString::getNumber(int& value, int base) const
{
	result_t	nresult = EINVAL;
	char*		endptr;
	long int	tmp;

	if ( m_nLength > 0 ) {
		tmp = _tstrtol(m_strData, &endptr, base);
		if ( *endptr == '\0' ) {
			value = (int)tmp;
			nresult = ESUCCESS;
		}
	}

	return nresult;
}

result_t CString::getNumber(unsigned int& value, int base) const
{
	result_t	nresult = EINVAL;
	char*		endptr;
	unsigned long int	tmp;

	if ( m_nLength > 0 ) {
		tmp = _tstrtoul(m_strData, &endptr, base);
		if ( *endptr == '\0' ) {
			value = (unsigned int)tmp;
			nresult = ESUCCESS;
		}
	}

	return nresult;
}

result_t CString::getNumber(int64_t& value, int base) const
{
	result_t		nresult = EINVAL;
	char*			endptr;
	long long int	tmp;

	if ( m_nLength > 0 ) {
		tmp = _tstrtoll(m_strData, &endptr, base);
		if ( *endptr == '\0' ) {
			value = (int64_t)tmp;
			nresult = ESUCCESS;
		}
	}

	return nresult;
}

result_t CString::getNumber(uint64_t& value, int base) const
{
	result_t		nresult = EINVAL;
	char*			endptr;
	long long unsigned int	tmp;

	if ( m_nLength > 0 ) {
		tmp = _tstrtoull(m_strData, &endptr, base);
		if ( *endptr == '\0' ) {
			value = (uint64_t)tmp;
			nresult = ESUCCESS;
		}
	}

	return nresult;
}

void CString::ltrim(const char* chars)
{
	if ( m_nLength > 0 )  {
		size_t		n;

		n = _tstrspn(m_strData, chars);
		if ( n > 0 ) {
			UNALIGNED_MEMCPY(m_strData, &m_strData[n], m_nLength-n);
			m_nLength -= n;
			m_strData[m_nLength] = '\0';
		}
	}
}

void CString::rtrim(const char* chars)
{
	if ( m_nLength > 0 )  {
		const char*		e = m_strData+m_nLength-1;

		while ( m_nLength > 0 && _tstrchr(chars, *e) != 0 )  {
			e--; m_nLength--;
		}

		m_strData[m_nLength] = '\0';
	}
}

/*
 * Parse substring from the string
 *
 * 		nOffset			offset in the string
 * 		nLength			substring length
 * 		strSubString	output substring
 */
void CString::subString(size_t nOffset, size_t nLength, CString& strSubString) const
{
	size_t	l = size(), lsub;

	if ( nLength == 0 || nOffset >= l )  {
		strSubString.clear();
	}
	else {
		lsub = l-nOffset;
		if ( lsub > nLength ) lsub = nLength;
		strSubString = CString(&m_strData[nOffset], lsub);
	}
}

/*
 * Insert sub-string to the string
 *
 *		nOffset			offset in the string to insert at
 *		strSubString	substring to insert
 */
void CString::insert(size_t nOffset, const char* strSubString)
{
	size_t	lsub = strSubString ? _tstrlen(strSubString) : 0;
	size_t	l = size(), ltail;

	if ( lsub > 0 )  {
		if ( nOffset < l )  {
			makeBuffer(l+lsub+1);

			ltail = l-nOffset;
			_tmemmove(&m_strData[nOffset+lsub], &m_strData[nOffset], ltail+1);
			_tmemmove(&m_strData[nOffset], strSubString, lsub);
			m_nLength += lsub;
		}
		else {
			append(strSubString);
		}
	}
}

/*
 * Allocate buffer to contains at least 'length' bytes
 *
 * 		length		minimum internal buffer size, bytes
 */
void CString::makeBuffer(size_t length)
{
	if ( m_nMaxLength < length )  {
		size_t 		n, newSize;
		void*		pBuffer;

		n = STRING_BUFFER_STEP*4;
		if ( length <= n ) {
			newSize = ALIGN(length, STRING_BUFFER_STEP);
		} else
		if ( length <= (n*4) )  {
			newSize = ALIGN(length, n);
		} else {
			newSize = ALIGN(length, 1024);
		}

		pBuffer = memRealloc(m_strData, newSize);
		if ( pBuffer )  {
			m_strData = (char*)pBuffer;
			m_nMaxLength = newSize;
		}
		else {
			throw std::bad_alloc();
		}
	}
}

/*
 * Dump string object
 *
 * 		strPref		string prefix, may be 0
 */
void CString::dump(const char* strPref) const
{
	log_dump("Dump%s: [%d/%d] %s\n", strPref ? strPref : "",
			 m_nLength, m_nMaxLength,
			 m_strData ? m_strData : "<NULL>");
}
