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
 *  Revision 1.0, 01.07.2016 10:35:56
 *      Initial revision.
 *
 */

#ifndef __CARBON_CSTRING_H_INCLUDED__
#define __CARBON_CSTRING_H_INCLUDED__

#include <stdarg.h>

#include "shell/types.h"
#include "shell/defines.h"
#include "shell/memory.h"
#include "shell/logger.h"
#include "shell/tstring.h"

class CString
{
	protected:
		char*		m_strData;			/* Current object value */
		size_t		m_nLength;			/* Current string length, bytes */
		size_t		m_nMaxLength;		/* Maximum allocated buffer length, bytes */

	public:
		CString(const char* strData = 0);
		CString(const void* memData, size_t size);
		CString(const CString& str);
		virtual ~CString();

	public:
		boolean_t isEmpty() const {
			return m_nLength == 0;
		}

		operator const char*() const {
			return m_strData ? m_strData : "";
		}

		const char* c_str() const {
			return m_strData ? m_strData : "";
		}

		const char* cs() const {
			return c_str();
		}

		CString& operator=(const CString& str) {
			if ( this != &str ) {
				copyString(str);
			}

			return *this;
		}

		CString& operator=(const char* strData) {
			copyString(strData);
			return *this;
		}

		CString& operator+=(const CString& str) {
			append(str);
			return *this;
		}

		CString& operator+=(const char* strData) {
			append(strData);
			return *this;
		}

		CString& operator+=(unsigned char c) {
			append(&c, 1);
			return *this;
		}

		friend CString operator+(const CString& str1, const CString& str2) {
			CString	sum = str1;
			sum += str2;
			return sum;
		}

		friend CString operator+(const CString& str1, const char* strData2) {
			CString	sum = str1;
			sum += strData2;
			return sum;
		}

		boolean_t equal(const CString* str) const;
		boolean_t equal(const char* strData) const;
		boolean_t equalNoCase(const CString* str) const;
		boolean_t equalNoCase(const char* strData) const;

		friend boolean_t operator==(const CString& str1, const CString& str2)
		{
			return str1.equal(str2);
		}

		friend boolean_t operator==(const CString& str1, const char* strData2)
		{
			return str1.equal(strData2);
		}

		friend boolean_t operator!=(const CString& str1, const CString& str2)
		{
			return !str1.equal(str2);
		}

		friend boolean_t operator!=(const CString& str1, const char* strData2)
		{
			return !str1.equal(strData2);
		}

		const char& operator[](int i) const
		{
			static char DUMMY; DUMMY = 0;
			return (i >= 0 && i < (int)m_nLength) ? m_strData[i] : DUMMY;
		}

		char& operator[](int i) {
			static char DUMMY; DUMMY = 0;
			return (i >= 0 && i < (int)m_nLength) ? m_strData[i] : DUMMY;
		}

		void clear()  {
			if ( m_strData && m_nLength > 0 ) {
				*m_strData = '\0';
			}
			m_nLength = 0;
		}

		void free() {
			SAFE_FREE(m_strData);
			m_strData = 0;
			m_nLength = m_nMaxLength = 0;
		}

		size_t size() const {
			return m_nLength;
		}

		void append(const void* memData, size_t size);
		void append(const char* strData);
		void appendPath(const char* strPath);

		void format(const char* strFormat, ...);
		result_t getNumber(int& value, int base = 0) const;
		result_t getNumber(unsigned int& value, int base = 0) const;
		result_t getNumber(int64_t& value, int base = 0) const;
		result_t getNumber(uint64_t& value, int base = 0) const;

		void ltrim(const char* chars = " \t");
		void rtrim(const char* chars = " \t");
		void trim(const char* chars = " \t") {
			rtrim(chars);
			ltrim(chars);
		}

		void subString(size_t nOffset, size_t nLength, CString& strSubString) const;
		void insert(size_t nOffset, const char* strSubString);

		void dump(const char* strPref = 0) const;

	protected:
		void copyString(const char* strData);
		void makeBuffer(size_t length);
};

#endif /* __CARBON_CSTRING_H_INCLUDED__ */
