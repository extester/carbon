/*
 *  Carbon framework
 *  JSON Object (based on jansson library)
 *
 *  Copyright (c) 2018 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 11.04.2018 11:14:18
 *      Initial revision.
 *
 *  Revision 1.1, 19.10.2018 13:13:36
 *  	Added support of 'null' data type.
 */

#ifndef __CARBON_JSON_H_INCLUDED__
#define __CARBON_JSON_H_INCLUDED__

#include <list>

#include <jansson.h>

#include "shell/config.h"
#include "shell/shell.h"
#include "shell/object.h"
#if CARBON_DEBUG_TRACK_OBJECT
#include "shell/track_object.h"
#endif /* CARBON_DEBUG_TRACK_OBJECT */
#include "carbon/cstring.h"

/*
 * JSON various options
 */
typedef enum {
	JSON_OPTION_NONE 			= 0,
	JSON_OPTION_PRETTY 			= 1,
	JSON_OPTION_COMPACT			= 2,
	JSON_OPTION_SAVE_SAFE		= 4		/* use intermediate temp file when saving */
} json_option_t;

#define JSON_PRETTY_INDENT		4

/*
 * Public json types (library types are used private in the class)
 */
typedef enum {
	JSON_ITEM_OBJECT	= JSON_OBJECT,
	JSON_ITEM_ARRAY 	= JSON_ARRAY,
	JSON_ITEM_STRING 	= JSON_STRING,
	JSON_ITEM_INTEGER	= JSON_INTEGER,
	JSON_ITEM_FLOAT		= JSON_REAL,
	JSON_ITEM_BOOLEAN	= JSON_TRUE,
	JSON_ITEM_NULL		= JSON_NULL
} json_item_type_t;

class CJsonItem;
typedef std::list<CJsonItem*>		json_item_list_t;

/*******************************************************************************
 * Item base class
 */

#if CARBON_DEBUG_TRACK_OBJECT
#define __CJsonItem_PARENT				: public CTrackObject<CJsonItem>
#else /* CARBON_DEBUG_TRACK_OBJECT */
#define __CJsonItem_PARENT
#endif /* CARBON_DEBUG_TRACK_OBJECT */

class CJsonItem __CJsonItem_PARENT
{
	friend class CJsonOA;
	friend class CJsonArray;
	friend class CJsonObject;

	protected:
		CString					m_strName;			/* Item name (optional) */
		json_item_type_t		m_type;				/* Item type, JSON_ITEM_xxx */
		CJsonItem*				m_pParent;			/* Parent Item, 0 - root item */

	protected:
		CJsonItem(const char* strName, json_item_type_t type) :
			m_strName(strName),
			m_type(type),
			m_pParent(0)
		{
		}

	public:
		virtual ~CJsonItem()
		{
		}

	public:
		json_item_type_t getType() const { return m_type; }
		boolean_t isString() const { return getType() == JSON_ITEM_STRING; }
		boolean_t isInteger() const { return getType() == JSON_ITEM_INTEGER; }
		boolean_t isBoolean() const { return getType() == JSON_ITEM_BOOLEAN; }
		boolean_t isFloat() const { return getType() == JSON_ITEM_FLOAT; }
		boolean_t isNull() const { return getType() == JSON_ITEM_NULL; }
		boolean_t isObject() const { return getType() == JSON_ITEM_OBJECT; }
		boolean_t isArray() const { return getType() == JSON_ITEM_ARRAY; }

		const char* getName() const { return m_strName; }
		CJsonItem* getParent() const { return m_pParent; }

		boolean_t isRoot() const { return getParent() == 0; }
		CJsonItem* getRoot();

		virtual void dump(const char* strPref = "") const = 0;

	protected:
		virtual result_t doLoad(json_t* pItem) = 0;
		virtual result_t doSave(json_t* pItem) const = 0;

		virtual CJsonItem* clone() const = 0;

		virtual void dumpIndent(int nIndent, char* strBuf, size_t maxLength) const;
		virtual void dump(int nIndent, boolean_t bLast) const = 0;
};

/*******************************************************************************
 * JSON item String class
 */
class CJsonString : public CJsonItem
{
	protected:
		CString			m_strValue;

	public:
		CJsonString(const char* strName = "", const char* strValue = "") :
			CJsonItem(strName, JSON_ITEM_STRING),
			m_strValue(strValue)
		{
		}

		CJsonString(const CJsonString& jsonString) :
			CJsonItem(jsonString.getName(), JSON_ITEM_STRING),
			m_strValue(jsonString)
		{
		}

		virtual ~CJsonString()
		{
		}

	public:
		void set(const char* strValue) { m_strValue = strValue; }
		const char* get() const { return m_strValue; }

		const char* c_str() const { return m_strValue; }
		const char* cs() const { return c_str(); }

		CJsonString& operator=(const char* strValue) {
			set(strValue);
			return *this;
		}

		CJsonString& operator=(const CJsonString& jsonString) {
			m_strName = jsonString.getName();
			set(jsonString);
			return *this;
		}

		operator const char*() const {
			return cs();
		}

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual result_t doLoad(json_t* pItem);
		virtual result_t doSave(json_t* pItem) const;

		virtual CJsonItem* clone() const {
			CJsonString*	pNewString = new CJsonString;
			*pNewString = *this;
			return pNewString;
		}

		virtual void dump(int nIndent, boolean_t bLast) const;
};

/*******************************************************************************
 * JSON item Signed Integer class
 */

#if !JSON_INTEGER_IS_LONG_LONG
#error JSON_INTEGER_IS_LONGLONG must be set!
#endif /* !JSON_INTEGER_IS_LONG_LONG */

class CJsonInteger : public CJsonItem
{
	protected:
		int64_t			m_nValue;

	public:
		CJsonInteger(const char* strName = "") :
			CJsonItem(strName, JSON_ITEM_INTEGER),
			m_nValue(0)
		{
		}

		CJsonInteger(const CJsonInteger& jsonInteger) :
			CJsonItem(jsonInteger.getName(), JSON_ITEM_INTEGER),
			m_nValue(jsonInteger)
		{
		}

		CJsonInteger(const char* strName, unsigned int nValue) :
			CJsonItem(strName, JSON_ITEM_INTEGER),
			m_nValue(nValue)
		{
		}

		CJsonInteger(const char* strName, int nValue) :
			CJsonItem(strName, JSON_ITEM_INTEGER),
			m_nValue(nValue)
		{
		}

#if 0
#if __WORDSIZE != 32
		CJsonInteger(const char* strName, uint32_t nValue) :
			CJsonItem(strName, JSON_ITEM_INTEGER),
			m_nValue((uint64_t)nValue)
		{
		}

		CJsonInteger(const char* strName, int32_t nValue) :
			CJsonItem(strName, JSON_ITEM_INTEGER),
			m_nValue((uint64_t)nValue)
		{
		}
#endif /* __WORDSIZE != 32 */
#endif
		CJsonInteger(const char* strName, uint64_t nValue) :
			CJsonItem(strName, JSON_ITEM_INTEGER),
			m_nValue((int64_t)nValue)
		{
		}

		CJsonInteger(const char* strName, int64_t nValue) :
			CJsonItem(strName, JSON_ITEM_INTEGER),
			m_nValue(nValue)
		{
		}

		virtual ~CJsonInteger()
		{
		}

	public:
		void set(int64_t nValue) { m_nValue = nValue; }
		int64_t get() const { return m_nValue; }

		explicit operator unsigned int() const { return (unsigned int)m_nValue; }
		explicit operator int() const { return (int)m_nValue; }
#if 0
#if __WORDSIZE != 32
		explicit operator uint32_t() const { return (uint32_t)m_nValue; }
		explicit operator int32_t() const { return (int32_t)m_nValue; }
#endif /* __WORDSIZE != 32 */
		explicit operator uint64_t() const { return (uint64_t)m_nValue; }
		operator int64_t() const { return m_nValue; }
#endif
	operator int64_t() const { return m_nValue; }

		CJsonInteger& operator=(unsigned int nValue) { m_nValue = nValue; return *this; }
		CJsonInteger& operator=(int nValue) { m_nValue = nValue; return *this; }
#if 0
#if __WORDSIZE != 32
		CJsonInteger& operator=(uint32_t nValue) { m_nValue = (uint64_t)nValue; return *this; }
		CJsonInteger& operator=(int32_t nValue) { m_nValue = (uint64_t)nValue; return *this; }
#endif /* __WORDSIZE != 32 */
#endif
		CJsonInteger& operator=(uint64_t nValue) { m_nValue = (int64_t)nValue; return *this; }
		CJsonInteger& operator=(int64_t nValue) { m_nValue = nValue; return *this; }

		CJsonInteger& operator=(const CJsonInteger& jsonInteger) {
			m_strName = jsonInteger.getName();
			set(jsonInteger);
			return *this;
		}

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual result_t doLoad(json_t* pItem);
		virtual result_t doSave(json_t* pItem) const;

		virtual CJsonItem* clone() const {
			CJsonInteger*	pNewInteger = new CJsonInteger;
			*pNewInteger = *this;
			return pNewInteger;
		}

		virtual void dump(int nIndent, boolean_t bLast) const;
};

/*******************************************************************************
 * JSON item Boolean class
 */
class CJsonBoolean : public CJsonInteger
{
	public:
		CJsonBoolean(const char* strName = "", boolean_t bValue = FALSE) :
			CJsonInteger(strName, (int64_t)bValue)
		{
			m_type = JSON_ITEM_BOOLEAN;
		}

		CJsonBoolean(const CJsonBoolean& jsonBoolean) :
			CJsonInteger(jsonBoolean.getName(), jsonBoolean.get())
		{
			m_type = JSON_ITEM_BOOLEAN;
		}

		virtual ~CJsonBoolean()
		{
		}

	public:
		void set(boolean_t bValue) { m_nValue = bValue; }
		boolean_t get() const { return m_nValue != 0; }

		operator boolean_t() const { return m_nValue != 0; }
		CJsonBoolean& operator=(boolean_t bValue) { m_nValue = bValue; return *this; }

		CJsonBoolean& operator=(const CJsonBoolean& jsonBoolean) {
			m_strName = jsonBoolean.getName();
			set(jsonBoolean);
			return *this;
		}

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual result_t doLoad(json_t* pItem);
		virtual result_t doSave(json_t* pItem) const;

		virtual CJsonItem* clone() const {
			CJsonBoolean*	pNewBoolean = new CJsonBoolean;
			*pNewBoolean = *this;
			return pNewBoolean;
		}

		virtual void dump(int nIndent, boolean_t bLast) const;
};

/*******************************************************************************
 * JSON item Float class
 */
class CJsonFloat : public CJsonItem
{
	protected:
		double			m_dValue;

	public:
		CJsonFloat(const char* strName = "", double dValue = 0.0) :
			CJsonItem(strName, JSON_ITEM_FLOAT),
			m_dValue(dValue)
		{
		}

		CJsonFloat(const CJsonFloat& jsonFloat) :
			CJsonItem(jsonFloat.getName(), JSON_ITEM_FLOAT),
			m_dValue(jsonFloat)
		{
		}

		virtual ~CJsonFloat()
		{
		}

	public:
		void set(double dValue) { m_dValue = dValue; }
		double get() const { return m_dValue; }

		operator double() const { return m_dValue; }
		explicit operator float() const { return (float)m_dValue; }

		CJsonFloat& operator=(double dValue) { m_dValue = dValue; return *this; }
		CJsonFloat& operator=(float fValue) { m_dValue = (double)fValue; return *this; }

		CJsonFloat& operator=(const CJsonFloat& jsonFloat) {
			m_strName = jsonFloat.getName();
			set(jsonFloat);
			return *this;
		}

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual result_t doLoad(json_t* pItem);
		virtual result_t doSave(json_t* pItem) const;

		virtual CJsonItem* clone() const {
			CJsonFloat*	pNewFloat = new CJsonFloat;
			*pNewFloat = *this;
			return pNewFloat;
		}

		virtual void dump(int nIndent, boolean_t bLast) const;
};

/*******************************************************************************
 * JSON item Null class
 */
class CJsonNull : public CJsonItem
{
	protected:

	public:
		CJsonNull(const char* strName = "") :
			CJsonItem(strName, JSON_ITEM_NULL)
		{
		}

		CJsonNull(const CJsonNull& jsonNull) :
			CJsonItem(jsonNull.getName(), JSON_ITEM_NULL)
		{
		}

		virtual ~CJsonNull()
		{
		}

	public:
		CJsonNull& operator=(const CJsonNull& jsonNull) {
			m_strName = jsonNull.getName();
			return *this;
		}

		virtual void dump(const char* strPref = "") const;

	protected:
		virtual result_t doLoad(json_t* pItem);
		virtual result_t doSave(json_t* pItem) const;

		virtual CJsonItem* clone() const {
			CJsonNull*	pNewNull = new CJsonNull;
			*pNewNull = *this;
			return pNewNull;
		}

		virtual void dump(int nIndent, boolean_t bLast) const;
};


/*******************************************************************************
 * JSON Object/Array Item base class
 */
class CJsonOA : public CJsonItem
{
	protected:
		json_item_list_t	m_item;

	protected:
		CJsonOA(const char* strName, json_item_type_t type) :
			CJsonItem(strName, type)
		{
		}

	public:
		virtual ~CJsonOA()
		{
			clear();
		}

	public:
		virtual size_t size() const { return m_item.size(); }
		virtual CJsonItem* operator[](size_t index) const;

		virtual void insert(CJsonItem* pJsonItem) = 0;
		virtual void clear();

		virtual result_t load(const char* strJson);
		virtual result_t load(const char* strJson, size_t nLength) = 0;
		virtual result_t save(CString& strJson, json_option_t options = JSON_OPTION_NONE) const = 0;

		virtual result_t loadFile(const char* strFilename, json_option_t options = JSON_OPTION_NONE);
		virtual result_t saveFile(const char* strFilename, json_option_t options = JSON_OPTION_PRETTY) const;

		virtual void copy(const CJsonOA& jsonOA);

		virtual void dump(int nIndent, boolean_t bLast) const = 0;

	protected:
		virtual result_t doLoad(json_t* pItem) = 0;
		virtual result_t doSave(json_t* pItem) const = 0;

		CJsonItem* createItem(const char* strName, json_type type) const;
		json_t* createJanssonItem(json_item_type_t type, boolean_t bValue) const;

		virtual CJsonItem* clone() const = 0;
};

/*******************************************************************************
 * JSON Object class
 */
class CJsonArray;

class CJsonObject : public CJsonOA
{
	public:
		CJsonObject(const char* strName = "") :
			CJsonOA(strName, JSON_ITEM_OBJECT)
		{
		}

		CJsonObject(const CJsonObject& jsonObject) :
			CJsonOA(jsonObject.getName(), JSON_ITEM_OBJECT)
		{
			copy(jsonObject);
		}

		virtual ~CJsonObject()
		{
		}

	public:
		CJsonItem* get(const char* strName) const;
		virtual void insert(CJsonItem* pJsonItem);
		result_t remove(const char* strName);

		CJsonString& insert(const char* strName, const char* strValue) {
			CJsonString*	pItem = new CJsonString(strName, strValue);
			insert(pItem);
			return *pItem;
		}

		CJsonInteger& insert(const char* strName, unsigned int nValue) {
			CJsonInteger*	pItem = new CJsonInteger(strName, nValue);
			insert(pItem);
			return *pItem;
		}

		CJsonInteger& insert(const char* strName, int nValue) {
			CJsonInteger*	pItem = new CJsonInteger(strName, nValue);
			insert(pItem);
			return *pItem;
		}

		CJsonInteger& insert(const char* strName, uint64_t nValue) {
			CJsonInteger*	pItem = new CJsonInteger(strName, nValue);
			insert(pItem);
			return *pItem;
		}

		CJsonInteger& insert(const char* strName, int64_t nValue) {
			CJsonInteger*	pItem = new CJsonInteger(strName, nValue);
			insert(pItem);
			return *pItem;
		}

		CJsonBoolean& insertBoolean(const char* strName, boolean_t bValue) {
			CJsonBoolean*	pItem = new CJsonBoolean(strName, bValue);
			insert(pItem);
			return *pItem;
		}

		CJsonNull& insertNull(const char* strName) {
			CJsonNull*	pItem = new CJsonNull(strName);
			insert(pItem);
			return *pItem;
		}

		CJsonFloat& insert(const char* strName, double dValue) {
			CJsonFloat*	pItem = new CJsonFloat(strName, dValue);
			insert(pItem);
			return *pItem;
		}

		CJsonObject& insertObject(const char* strName) {
			CJsonObject*	pItem = new CJsonObject(strName);
			insert(pItem);
			return *pItem;
		}

		CJsonArray& insertArray(const char* strName);

		boolean_t isExists(const char* strName) const {
			return get(strName) != 0;
		}

		CJsonItem* operator[](const char* strName) const {
			return get(strName);
		}

		CJsonItem* operator[](size_t index) const {
			return CJsonOA::operator[](index);
		}

		CJsonObject& operator=(const CJsonObject& jsonObj) {
			m_strName = jsonObj.getName();
			copy(jsonObj);
			return *this;
		}

		CJsonItem* find(const char* strName) const {
			return get(strName);
		}

		result_t getValue(const char* strName, CString& strValue) const {
			CJsonItem*		pItem = find(strName);
			CJsonString*	pJsonString = dynamic_cast<CJsonString*>(pItem);
			result_t		nresult = ESUCCESS;

			if ( pJsonString )  { strValue = *pJsonString; }
			else { nresult = pItem ? EINVAL : ENOENT; }
			return nresult;
		}

		result_t getValue(const char* strName, unsigned int& nValue) const {
			CJsonItem*		pItem = find(strName);
			CJsonInteger*	pJsonInteger = dynamic_cast<CJsonInteger*>(pItem);
			result_t		nresult = ESUCCESS;

			if ( pJsonInteger )  { nValue = *pJsonInteger; }
			else { nresult = pItem ? EINVAL : ENOENT; }
			return nresult;
		}

		result_t getValue(const char* strName, int& nValue) const {
			CJsonItem*		pItem = find(strName);
			CJsonInteger*	pJsonInteger = dynamic_cast<CJsonInteger*>(pItem);
			result_t		nresult = ESUCCESS;

			if ( pJsonInteger )  { nValue = *pJsonInteger; }
			else { nresult = pItem ? EINVAL : ENOENT; }
			return nresult;
		}

		result_t getValue(const char* strName, uint64_t& nValue) const {
			CJsonItem*		pItem = find(strName);
			CJsonInteger*	pJsonInteger = dynamic_cast<CJsonInteger*>(pItem);
			result_t		nresult = ESUCCESS;

			if ( pJsonInteger )  { nValue = (uint64_t)*pJsonInteger; }
			else { nresult = pItem ? EINVAL : ENOENT; }
			return nresult;
		}

		result_t getValue(const char* strName, int64_t& nValue) const {
			CJsonItem*		pItem = find(strName);
			CJsonInteger*	pJsonInteger = dynamic_cast<CJsonInteger*>(pItem);
			result_t		nresult = ESUCCESS;

			if ( pJsonInteger )  { nValue = *pJsonInteger; }
			else { nresult = pItem ? EINVAL : ENOENT; }
			return nresult;
		}

		result_t getValue(const char* strName, double& dValue) const {
			CJsonItem*		pItem = find(strName);
			CJsonFloat*		pJsonFloat = dynamic_cast<CJsonFloat*>(pItem);
			result_t		nresult = ESUCCESS;

			if ( pJsonFloat )  { dValue = *pJsonFloat; }
			else { nresult = pItem ? EINVAL : ENOENT; }
			return nresult;
		}

		CJsonObject* getObject(const char* strName) const {
			return dynamic_cast<CJsonObject*>(find(strName));
		}

		CJsonArray* getArray(const char* strName) const;

		virtual result_t load(const char* strJson, size_t nLength);
		virtual result_t load(const char* strJson) { return CJsonOA::load(strJson); }
		virtual result_t save(CString& strJson, json_option_t options = JSON_OPTION_NONE) const;

		virtual void dump(const char* strPref = "") const;
		virtual void dump(int nIndent, boolean_t bLast) const;

	protected:
		virtual result_t doLoad(json_t* pItem);
		virtual result_t doSave(json_t* pItem) const;

		virtual CJsonItem* clone() const {
			CJsonObject*	pNewObject = new CJsonObject(getName());
			pNewObject->copy(*this);
			return pNewObject;
		}
};

/*******************************************************************************
 * JSON Array class
 */
class CJsonArray : public CJsonOA
{
	public:
		CJsonArray(const char* strName = "") :
			CJsonOA(strName, JSON_ITEM_ARRAY)
		{
		}

		CJsonArray(const CJsonArray& jsonArray) :
			CJsonOA(jsonArray.getName(), JSON_ITEM_ARRAY)
		{
			copy(jsonArray);
		}

		virtual ~CJsonArray()
		{
		}

	public:
		CJsonItem* get(size_t index) const {
			return operator[](index);
		}

		virtual void insert(CJsonItem* pJsonItem) {
			insert(pJsonItem, (size_t)-1);
		}
		virtual CJsonItem* insert(CJsonItem* pJsonItem, size_t index);
		result_t remove(size_t index);

		CJsonString* insert(const char* strValue, size_t index = (size_t)-1) {
			CJsonString*	pItem = new CJsonString("", strValue);
			CJsonItem*		x = insert(pItem, index);
			if ( !x ) 		SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonInteger* insert(unsigned int nValue, size_t index = (size_t)-1) {
			CJsonInteger*	pItem = new CJsonInteger("", nValue);
			CJsonItem*		x = insert(pItem, index);
			if ( !x ) 		SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonInteger* insert(int nValue, size_t index = (size_t)-1) {
			CJsonInteger*	pItem = new CJsonInteger("", nValue);
			CJsonItem*		x = insert(pItem, index);
			if ( !x ) 		SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonInteger* insert(uint64_t nValue, size_t index = (size_t)-1) {
			CJsonInteger*	pItem = new CJsonInteger("", nValue);
			CJsonItem*		x = insert(pItem, index);
			if ( !x ) 		SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonInteger* insert(int64_t nValue, size_t index = (size_t)-1) {
			CJsonInteger*	pItem = new CJsonInteger("", nValue);
			CJsonItem*		x = insert(pItem, index);
			if ( !x ) 		SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonBoolean* insertBoolean(boolean_t bValue, size_t index = (size_t)-1) {
			CJsonBoolean*	pItem = new CJsonBoolean("", bValue);
			CJsonItem*		x = insert(pItem, index);
			if ( !x ) 		SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonNull* insertNull(size_t index = (size_t)-1) {
			CJsonNull*	pItem = new CJsonNull;
			CJsonItem*	x = insert(pItem, index);
			if ( !x ) 	SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonFloat* insert(double dValue, size_t index = (size_t)-1) {
			CJsonFloat*	pItem = new CJsonFloat("", dValue);
			CJsonItem*		x = insert(pItem, index);
			if ( !x ) 		SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonObject* insertObject(size_t index = (size_t)-1) {
			CJsonObject*	pItem = new CJsonObject;
			CJsonItem*		x = insert(pItem, index);
			if ( !x ) 		SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonArray* insertArray(size_t index = (size_t)-1) {
			CJsonArray*		pItem = new CJsonArray;
			CJsonItem*		x = insert(pItem, index);
			if ( !x ) 		SAFE_DELETE(pItem);
			return pItem;
		}

		CJsonItem* operator[](size_t index) const {
			return CJsonOA::operator[](index);
		}

		CJsonArray& operator=(const CJsonArray& jsonArr) {
			m_strName = jsonArr.getName();
			copy(jsonArr);
			return *this;
		}

		virtual result_t load(const char* strJson, size_t nLength);
		virtual result_t load(const char* strJson) { return CJsonOA::load(strJson); }
		virtual result_t save(CString& strJson, json_option_t options = JSON_OPTION_NONE) const;

		virtual void dump(const char* strPref = "") const;
		virtual void dump(int nIndent, boolean_t bLast) const;

	protected:
		virtual result_t doLoad(json_t* pItem);
		virtual result_t doSave(json_t* pItem) const;

		virtual CJsonItem* clone() const {
			CJsonArray*	pNewArray = new CJsonArray(getName());
			pNewArray->copy(*this);
			return pNewArray;
		}
};

#endif /* __CARBON_JSON_H_INCLUDED__ */
