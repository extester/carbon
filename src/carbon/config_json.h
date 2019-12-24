/*
 *  Data acquisition project
 *  Json configuration base class
 *
 *  Copyright (c) 2017 Ritm. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 21.11.2017 12:28:03
 *      Initial revision.
 */

#ifndef __CARBON_CONFIG_JSON_H_INCLUDED__
#define __CARBON_CONFIG_JSON_H_INCLUDED__

#include <list>

#include "shell/object.h"

#include "carbon/json.h"

typedef enum {
	CONFIG_ITEM_OBJECT,
	CONFIG_ITEM_ARRAY,
	CONFIG_ITEM_STRING,
	CONFIG_ITEM_INTEGER
} config_item_type_t;

class CConfigItem;
typedef std::list<CConfigItem*>		config_item_list_t;

/*******************************************************************************
 * Item base class
 */
class CConfigItem
{
	friend class CConfigObject;
	friend class CConfigArray;

	protected:
		CString				m_strName;
		config_item_type_t	m_type;
		CConfigItem*		m_pParent;
		boolean_t			m_bModified;

	protected:
		CConfigItem(const char* strName, config_item_type_t type) :
			m_strName(strName),
			m_type(type),
			m_pParent(0),
			m_bModified(FALSE)
		{
		}

	public:
		virtual ~CConfigItem()
		{
		}

	public:
		config_item_type_t getType() const { return m_type; }
		const char* getName() const { return m_strName; }
		CConfigItem* getParent() const { return m_pParent; }

		boolean_t isRoot() const { return getParent() == 0; }
		CConfigItem* getRoot();

		void setModified(boolean_t bModified = TRUE) { getRoot()->m_bModified = bModified; }
		boolean_t isMofdified() { return getRoot()->m_bModified; }

		virtual result_t load(const CJsonItem* pJsonItem) = 0;
		virtual result_t save(CJsonItem* pJsonItem) const = 0;

		virtual void dump(int nIndent, boolean_t bLast) const = 0;
		virtual void dumpIndent(int nIndent, char* strBuf, size_t maxLength) const;
};

/*******************************************************************************
 * Config String class
 */
class CConfigString : public CConfigItem
{
	protected:
		CString			m_strValue;

	public:
		CConfigString(const char* strName, const char* strValue = "") :
			CConfigItem(strName, CONFIG_ITEM_STRING),
			m_strValue(strValue)
		{
		}

		virtual ~CConfigString()
		{
		}

	public:
		const char* get() const { return m_strValue; }
		const char* c_str() const { return m_strValue; }
		const char* cs() const { return m_strValue; }
		boolean_t isEmpty() const { return m_strValue.isEmpty(); }

		void set(const char* strValue) { m_strValue = strValue; setModified(); }

		CConfigString& operator=(const char* strValue) {
			set(strValue);
			return *this;
		}

		operator const char*() const { return get(); }

		virtual result_t load(const CJsonItem* pJsonItem);
		virtual result_t save(CJsonItem* pJsonItem) const;

		virtual void dump(int nIndent, boolean_t bLast) const;
};

/*******************************************************************************
 * Config Integer class
 */
class CConfigInteger : public CConfigItem
{
	protected:
		int64_t		m_nValue;

	public:
		CConfigInteger(const char* strName) :
			CConfigItem(strName, CONFIG_ITEM_INTEGER),
			m_nValue(0)
		{
		}

		CConfigInteger(const char* strName, int64_t nValue) :
			CConfigItem(strName, CONFIG_ITEM_INTEGER),
			m_nValue(nValue)
		{
		}

		virtual ~CConfigInteger()
		{
		}

	public:
		explicit operator uint64_t() const { return (uint64_t)m_nValue; }
		explicit operator int64_t() const { return m_nValue; }
		explicit operator int() const { return (int)m_nValue; }
		explicit operator unsigned int() const { return (unsigned int)m_nValue; }

		CConfigInteger& operator=(uint64_t nValue) {
			m_nValue = (int64_t)nValue;
			setModified();
			return *this;
		}

		CConfigInteger& operator=(int64_t nValue) {
			m_nValue = nValue;
			setModified();
			return *this;
		}

		CConfigInteger& operator=(int nValue) {
			m_nValue = (int64_t)nValue;
			setModified();
			return *this;
		}

		CConfigInteger& operator=(unsigned int nValue) {
			m_nValue = (int64_t)nValue;
			setModified();
			return *this;
		}

		virtual result_t load(const CJsonItem* pJsonItem);
		virtual result_t save(CJsonItem* pJsonItem) const;

		virtual void dump(int nIndent, boolean_t bLast) const;
};

/*******************************************************************************
 * Object/Array Item base class
 */
class CConfigOA : public CConfigItem
{
	protected:
		config_item_list_t	m_item;

	protected:
		CConfigOA(const char* strName, config_item_type_t type) :
			CConfigItem(strName, type)
		{
		}

	public:
		virtual ~CConfigOA()
		{
			removeAll();
		}

	public:
		virtual size_t size() const { return m_item.size(); }
		virtual CConfigItem* operator[](size_t index) const;
		virtual result_t setAt(size_t index, CConfigItem* pItem);

		virtual result_t insert(CConfigItem* pItem) = 0;
		virtual void removeAll();

		virtual result_t load(const CJsonItem* pJsonItem) = 0;
		virtual result_t save(CJsonItem* pJsonItem) const = 0;

		virtual void dump(int nIndent, boolean_t bLast) const = 0;

	protected:

		CConfigItem* _createItem(const char* strName, json_item_type_t type) const;
		CJsonItem* _createJsonItem(const char* strName, config_item_type_t type) const;

	private:
};

/*******************************************************************************
 * Config Object class
 */
class CConfigObject : public CConfigOA
{
	public:
		CConfigObject(const char* strName) :
			CConfigOA(strName, CONFIG_ITEM_OBJECT)
		{
		}

		virtual ~CConfigObject()
		{
		}

	public:
		virtual result_t insert(CConfigItem* pItem);

		virtual result_t insertObject(const char* strName)  {
			CConfigObject*	pObject = new CConfigObject(strName);
			result_t		nresult = insert(pObject);
			if ( nresult != ESUCCESS ) { delete pObject; }
			return nresult;
		}

		virtual result_t insertArray(const char* strName);

		result_t remove(CConfigItem* pItem);
		result_t remove(const char* strName);

		virtual CConfigItem* find(const char* strName) const;
		CConfigItem* operator[](const char* strName) const {
			return find(strName);
		}

		CConfigItem* operator[](size_t index) const {
			return CConfigOA::operator[](index);
		}

		virtual result_t load(const CJsonItem* pJsonItem);
		virtual result_t save(CJsonItem* pJsonItem) const;

		virtual void dump(int nIndent, boolean_t bLast) const;
};

/*******************************************************************************
 * Config Array class
 */
class CConfigArray : public CConfigOA
{
	public:
		CConfigArray(const char* strName) :
			CConfigOA(strName, CONFIG_ITEM_ARRAY)
		{
		}

		virtual ~CConfigArray()
		{
		}

	public:
		CConfigItem* operator[](size_t index) const {
			return CConfigOA::operator[](index);
		}

		virtual result_t insert(CConfigItem* pItem) {
			return insert(pItem, -1);
		}
		virtual result_t insert(CConfigItem* pItem, size_t index);

		virtual result_t insertObject(size_t index = (size_t)-1)  {
			CConfigObject*	pObject = new CConfigObject("");
			result_t		nresult = insert(pObject, index);
			if ( nresult != ESUCCESS ) { delete pObject; }
			return nresult;
		}

		result_t remove(size_t index);
		result_t remove(CConfigItem* pItem);

		virtual result_t load(const CJsonItem* pJsonItem);
		virtual result_t save(CJsonItem* pJsonItem) const;

		virtual void dump(int nIndent, boolean_t bLast) const;
};


/*******************************************************************************
 * Configuration base class
 */
class CConfigJson : public CObject
{
	protected:
		CString				m_strFile;		/* Configuration filename */
		CConfigOA*			m_pRoot;

	public:
		CConfigJson(const char* strFile);
		virtual ~CConfigJson();

	public:
		boolean_t isModified() const { return m_pRoot ? m_pRoot->isMofdified() : FALSE; }
		void setModified(boolean_t bModified = FALSE) { if ( m_pRoot ) m_pRoot->setModified(bModified); }

		virtual void clear();
		void setFilename(const char* strFilename) { m_strFile = strFilename; }

		virtual CConfigObject* getConfigObject(const char* strName = NULL) const;
		virtual CConfigArray* getConfigArray(const char* strName = NULL) const;

		//virtual result_t load(const CJsonItem* pJsonItem);
		virtual result_t load();
		virtual result_t save(int options = 0);
		virtual result_t update(int options = 0);

	public:
		virtual void dump(const char* strPref = "") const;
};

#define CONFIG_OPTION_READABLE		JSON_OPTION_PRETTY
#define CONFIG_OPTION_SAVE_SAFE		JSON_OPTION_SAVE_SAFE
#define CONFIG_OPTION_COMPACT		JSON_OPTION_COMPACT

#endif /* __CARBON_CONFIG_JSON_H_INCLUDED__ */
