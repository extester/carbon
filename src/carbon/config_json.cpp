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
 *  Revision 1.0, 21.11.2017 15:09:19
 *      Initial revision.
 *
 *  Revision 1.1, 16.03.2018 17:25:20
 *  	Fixed bug in CConfigObject::save().
 */

#include "stdexcept"

#include "shell/file.h"

#include "carbon/json.h"
#include "carbon/logger.h"
#include "carbon/config_json.h"

#define CONFIG_DUMP_INDENT			JSON_PRETTY_INDENT

/*******************************************************************************
 * CConfigItem class
 */

CConfigItem* CConfigItem::getRoot()
{
	CConfigItem		*pItem, *pParent;

	pItem = this;
	while ( (pParent=pItem->getParent()) != 0 )  {
		pItem = pParent;
	}

	return pItem;
}

void CConfigItem::dumpIndent(int nIndent, char* strBuf, size_t maxLength) const
{
	int 	i, n = MIN(nIndent, (int)maxLength);

	for(i=0; i<n; i++)  {
		strBuf[i] = ' ';
	}
	strBuf[n] = '\0';
}

/*******************************************************************************
 * CConfigString class
 */

result_t CConfigString::load(const CJsonItem* pJsonItem)
{
	result_t	nresult = ESUCCESS;

	if ( pJsonItem->getType() == JSON_ITEM_STRING ) {
		m_strValue = *dynamic_cast<const CJsonString*>(pJsonItem);
	}
	else {
		log_error(L_GEN, "[confstr] json type %d, expected STRING(%d)\n",
				  pJsonItem->getType(), JSON_ITEM_STRING);
		nresult = EINVAL;
	}

	return nresult;
}

result_t CConfigString::save(CJsonItem* pJsonItem) const
{
	result_t	nresult = ESUCCESS;

	shell_assert(pJsonItem);

	if ( pJsonItem->isString() ) {
		*dynamic_cast<CJsonString*>(pJsonItem) = m_strValue;
	}
	else {
		log_error(L_GEN, "[confstr] json type %d, expected STRING(%d)\n",
				  pJsonItem->getType(), JSON_ITEM_STRING);
		nresult = EINVAL;
	}

	return nresult;
}

void CConfigString::dump(int nIndent, boolean_t bLast) const
{
	char			strBuf[256];
	size_t			l, length;
	boolean_t		bParentArray;

	bParentArray = (getParent() != 0 && getParent()->getType() == CONFIG_ITEM_ARRAY);

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	if ( !bParentArray )  {
		_tsnprintf(&strBuf[l], length-l, "\"%s\": \"%s\"%s\n",
				   getName(), m_strValue.cs(), bLast ? "" : ",");
	}
	else {
		_tsnprintf(&strBuf[l], length-1, "\"%s\"%s\n",
				   m_strValue.cs(), bLast ? "" : ",");
	}

	log_dump(strBuf);
}

/*******************************************************************************
 * CConfigInteger
 */

result_t CConfigInteger::load(const CJsonItem* pJsonItem)
{
	result_t	nresult = ESUCCESS;

	if ( pJsonItem->getType() == JSON_ITEM_INTEGER ) {
		m_nValue = *dynamic_cast<const CJsonInteger*>(pJsonItem);
	}
	else {
		log_error(L_GEN, "[confstr] json type %d, expected INTEGER(%d)\n",
				  pJsonItem->getType(), JSON_ITEM_INTEGER);
		nresult = EINVAL;
	}

	return nresult;
}

result_t CConfigInteger::save(CJsonItem* pJsonItem) const
{
	result_t	nresult = ESUCCESS;

	shell_assert(pJsonItem);

	if ( pJsonItem->isInteger() ) {
		*dynamic_cast<CJsonInteger*>(pJsonItem) = m_nValue;
	}
	else {
		log_error(L_GEN, "[confstr] json type %d, expected INTEGER(%d)\n",
				  pJsonItem->getType(), JSON_ITEM_INTEGER);
		nresult = EINVAL;
	}

	return nresult;
}

void CConfigInteger::dump(int nIndent, boolean_t bLast) const
{
	char			strBuf[256];
	size_t			l, length;
	boolean_t		bParentArray;

	bParentArray = (getParent() != 0 && getParent()->getType() == CONFIG_ITEM_ARRAY);

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	if ( !bParentArray )  {
		_tsnprintf(&strBuf[l], length-l, "\"%s\": %" PRIi64 "%s\n",
			getName(), m_nValue, bLast ? "" : ",");
	}
	else {
		_tsnprintf(&strBuf[l], length-1, "\"%" PRIi64 "\"%s\n",
			m_nValue, bLast ? "" : ",");
	}

	log_dump(strBuf);
}


/*******************************************************************************
 * CConfigOA class
 */

void CConfigOA::removeAll()
{
	while ( !m_item.empty() )  {
		delete *m_item.begin();
		m_item.erase(m_item.begin());
	}
	setModified();
}

CConfigItem* CConfigOA::operator[](size_t index) const
{
	if ( index < m_item.size() ) {
		config_item_list_t::const_iterator	it;
		size_t	i;

		it = m_item.begin();
		for(i=0; i<index; i++)  {
			it++;
		}

		return *it;
	}

	return 0;
}

result_t CConfigOA::setAt(size_t index, CConfigItem* pItem)
{
	result_t	nresult = ESUCCESS;

	shell_assert(index <= size());

	if ( index < size() )  {
		config_item_list_t::iterator	it;
		size_t	i;

		it = m_item.begin();
		for(i=0; i<index; i++)  {
			it++;
		}

		delete *it;
		*it = pItem;
		setModified();
	}
	else if ( index == size() ) {
		nresult = insert(pItem);
	}
	else {
		log_error(L_GEN, "[confbase] array index %lu out of range %lu\n", index, size());
		nresult = EINVAL;
	}

	return nresult;
}

CConfigItem* CConfigOA::_createItem(const char* strName, json_item_type_t type) const
{
	CConfigItem*	pConfigItem;

	switch ( type ) {
		case JSON_ITEM_OBJECT:
			pConfigItem = new CConfigObject(strName);
			break;

		case JSON_ITEM_ARRAY:
			pConfigItem = new CConfigArray(strName);
			break;

		case JSON_ITEM_STRING:
			pConfigItem = new CConfigString(strName);
			break;

		case JSON_ITEM_INTEGER:
		case JSON_ITEM_BOOLEAN:
			pConfigItem = new CConfigInteger(strName);
			break;

		case JSON_ITEM_FLOAT:
		case JSON_ITEM_NULL:
		default:
			log_error(L_GEN, "[confbase] '%s': unsupported JSON element type %d\n",
					  	getName(), type);
			pConfigItem = 0;
			break;
	}

	return pConfigItem;
}

CJsonItem* CConfigOA::_createJsonItem(const char* strName, config_item_type_t type) const
{
	CJsonItem*		pJsonItem = 0;

	switch ( type ) {
		case CONFIG_ITEM_OBJECT:
			pJsonItem = new CJsonObject(strName);
			break;

		case CONFIG_ITEM_ARRAY:
			pJsonItem = new CJsonArray(strName);
			break;

		case CONFIG_ITEM_STRING:
			pJsonItem = new CJsonString(strName);
			break;

		case CONFIG_ITEM_INTEGER:
			pJsonItem = new CJsonInteger(strName);
			break;

		default:
			log_error(L_GEN, "[confbase] unknown config item type %d\n", type);
			break;
	}

	return pJsonItem;
}

/*******************************************************************************
 * CConfigObject class
 */

result_t CConfigObject::insertArray(const char* strName)
{
	CConfigArray*	pArray = new CConfigArray(strName);
	result_t		nresult = insert(pArray);
	if ( nresult != ESUCCESS ) { delete pArray; }
	return nresult;
}

result_t CConfigObject::insert(CConfigItem* pItem)
{
	CConfigItem*	pTmpItem;

	pTmpItem = find(pItem->getName());
	if ( pTmpItem )  {
		remove(pTmpItem);
	}

	pItem->m_pParent = this;
	m_item.push_back(pItem);
	setModified();

	return ESUCCESS;
}

result_t CConfigObject::remove(CConfigItem* pItem)
{
	config_item_list_t::iterator	it;
	result_t	nresult = ENOENT;

	it = m_item.begin();
	while ( it != m_item.end() )  {
		if ( *it == pItem )  {
			delete *it;
			m_item.erase(it);
			nresult = ESUCCESS;
			setModified();
			break;
		}

		it++;
	}

	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[confbase] '%s': item %s is not found\n",
				  	getName(), pItem ? pItem->getName() : "\"\"");
	}

	return nresult;
}

result_t CConfigObject::remove(const char* strName)
{
	config_item_list_t::iterator	it;
	CString		sName(strName);
	result_t	nresult = ENOENT;

	it = m_item.begin();
	while ( it != m_item.end() ) {
		if ( sName == (*it)->getName() )  {
			delete *it;
			m_item.erase(it);
			nresult = ESUCCESS;
			setModified();
			break;
		}

		it++;
	}

	if ( nresult != ESUCCESS )  {
		log_debug(L_GEN, "[confbase] '%s': item %s is not found\n", getName(), strName);
	}

	return nresult;
}

CConfigItem* CConfigObject::find(const char* strName) const
{
	config_item_list_t::const_iterator	it;
	CConfigItem*	pItem = 0;
	CString			sName(strName);

	it = m_item.begin();
	while ( it != m_item.end() ) {
		if ( sName == (*it)->getName() )  {
			pItem = *it;
			break;
		}

		it++;
	}

	return pItem;
}

result_t CConfigObject::load(const CJsonItem* pJsonItem)
{
	CConfigItem*		pConfigItem;
	const CJsonObject*	pJsonObj;
	CJsonItem*			pItem;
	size_t				i, count;
	result_t			nresult = ESUCCESS;

	shell_assert(pJsonItem);
	shell_assert(size() == 0);

	if ( !pJsonItem->isObject() )  {
		return EINVAL;
	}

	pJsonObj = dynamic_cast<const CJsonObject*>(pJsonItem);

	count = pJsonObj->size();
	for(i=0; i<count; i++) {
		pItem = (*pJsonObj)[i];
		if ( pItem ) {
			pConfigItem = _createItem(pItem->getName(), pItem->getType());
			if ( pConfigItem == 0 )  {
				nresult = EINVAL;
				break;
			}

			nresult = pConfigItem->load(pItem);
			if ( nresult == ESUCCESS )  {
				insert(pConfigItem);
			}
			else {
				delete pConfigItem;
				break;
			}
		}
	}

	return nresult;
}

result_t CConfigObject::save(CJsonItem* pJsonItem) const
{
	config_item_list_t::const_iterator	it;
	CJsonObject*	pJsonObj;
	CJsonItem*		pIterItem;
	result_t		nresult = ESUCCESS;

	shell_assert(pJsonItem);

	if ( !pJsonItem->isObject() ) {
		return EINVAL;
	}

	pJsonObj = dynamic_cast<CJsonObject*>(pJsonItem);
	shell_assert(pJsonObj->size() == 0);

	it = m_item.begin();
	while ( it != m_item.end() ) {
		pIterItem = _createJsonItem((*it)->getName(), (*it)->getType());
		if ( !pIterItem )  {
			nresult = EINVAL;
			break;
		}

		nresult = (*it)->save(pIterItem);
		if ( nresult == ESUCCESS )  {
			pJsonObj->insert(pIterItem);
		}
		else {
			delete pIterItem;
			break;
		}

		it++;
	}

	return nresult;
}

void CConfigObject::dump(int nIndent, boolean_t bLast) const
{
	char			strBuf[256];
	size_t			l, length;
	config_item_list_t::const_iterator it;
	const CConfigItem	*pItem, *pParent;
	boolean_t 		hasName;

	hasName = CString(getName()) != "/";
	if ( hasName ) {
		pParent = getParent();
		hasName = pParent != 0 && pParent->getType() != CONFIG_ITEM_ARRAY;
	}

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	if ( hasName ) {
		_tsnprintf(&strBuf[l], length-l, "\"%s\": {\n", getName());
	}
	else {
		_tsnprintf(&strBuf[l], length-l, "{\n");
	}
	log_dump(strBuf);


	it = m_item.begin();
	while ( it != m_item.end() ) {
		pItem = *it;
		it++;
		pItem->dump(nIndent + CONFIG_DUMP_INDENT, it == m_item.end());
	}

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	_tsnprintf(&strBuf[l], length-l, "}%s\n", bLast ? "" : ",");
	log_dump(strBuf);
}


/*******************************************************************************
 * CConfigArray class
 */

result_t CConfigArray::insert(CConfigItem* pItem, size_t index)
{
	if ( index < size() )  {
		config_item_list_t::iterator	it;
		size_t	i;

		it = m_item.begin();
		for(i=0; i<index; i++)  {
			it++;
		}

		pItem->m_pParent = this;
		m_item.insert(it, pItem);
	}
	else {
		pItem->m_pParent = this;
		m_item.push_back(pItem);
	}

	setModified();
	return ESUCCESS;
}

result_t CConfigArray::remove(size_t index)
{
	result_t	nresult = EINVAL;

	if ( index < size() ) {
		config_item_list_t::iterator	it;
		size_t	i;

		it = m_item.begin();
		for (i = 0; i < index; i++) {
			it++;
		}

		delete *it;
		m_item.erase(it);
		setModified();
		nresult = ESUCCESS;
	}

	return nresult;
}

result_t CConfigArray::remove(CConfigItem* pItem)
{
	config_item_list_t::iterator	it;
	result_t	nresult = ENOENT;

	if ( pItem )  {
		it = m_item.begin();

		while ( it != m_item.end() )  {
			if ( *it == pItem )  {
				delete *it;
				m_item.erase(it);
				setModified();
				nresult = ESUCCESS;
				break;
			}
			it++;
		}
	}

	return nresult;
}

result_t CConfigArray::load(const CJsonItem* pJsonItem)
{
	CConfigItem*		pConfigItem;
	const CJsonArray*	pJsonArr;
	CJsonItem*			pItem;
	size_t				i, count;
	result_t			nresult = ESUCCESS;

	shell_assert(pJsonItem);
	shell_assert(size() == 0);

	if ( !pJsonItem->isArray() )  {
		return EINVAL;
	}

	pJsonArr = dynamic_cast<const CJsonArray*>(pJsonItem);

	count = pJsonArr->size();
	for(i=0; i<count; i++) {
		pItem = (*pJsonArr)[i];
		if ( pItem ) {
			pConfigItem = _createItem(pItem->getName(), pItem->getType());
			if ( pConfigItem == 0 )  {
				nresult = EINVAL;
				break;
			}

			nresult = pConfigItem->load(pItem);
			if ( nresult == ESUCCESS )  {
				insert(pConfigItem);
			}
			else {
				delete pConfigItem;
				break;
			}
		}
	}

	return nresult;
}


result_t CConfigArray::save(CJsonItem* pJsonItem) const
{
	config_item_list_t::const_iterator	it;
	CJsonArray*		pJsonArr;
	CJsonItem*		pIterItem;
	result_t		nresult = ESUCCESS;

	shell_assert(pJsonItem);

	if ( !pJsonItem->isArray() ) {
		return EINVAL;
	}

	pJsonArr = dynamic_cast<CJsonArray*>(pJsonItem);
	shell_assert(pJsonArr->size() == 0);

	it = m_item.begin();
	while ( it != m_item.end() ) {
		pIterItem = _createJsonItem("", (*it)->getType());
		if ( !pIterItem )  {
			nresult = EINVAL;
			break;
		}

		nresult = (*it)->save(pIterItem);
		if ( nresult == ESUCCESS )  {
			pJsonArr->insert(pIterItem);
		}
		else {
			delete pIterItem;
			break;
		}

		it++;
	}

	return nresult;
}

void CConfigArray::dump(int nIndent, boolean_t bLast) const
{
	char			strBuf[256];
	size_t			l, length;
	config_item_list_t::const_iterator it;
	CConfigItem*	pItem;

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	if ( CString(getName()) != "/" ) {
		_tsnprintf(&strBuf[l], length-l, "\"%s\": [\n", getName());
	}
	else {
		_tsnprintf(&strBuf[l], length-l, "[\n");
	}
	log_dump(strBuf);


	it = m_item.begin();
	while ( it != m_item.end() ) {
		pItem = *it;
		it++;
		pItem->dump(nIndent + CONFIG_DUMP_INDENT, it == m_item.end());
	}

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	_tsnprintf(&strBuf[l], length-l, "]%s\n", bLast ? "" : ",");
	log_dump(strBuf);
}



/*******************************************************************************
 * CConfigJson class
 */

CConfigJson::CConfigJson(const char* strFile) :
	CObject("configuration"),
	m_strFile(strFile),
	m_pRoot(0)
{
}

CConfigJson::~CConfigJson()
{
	clear();
}

void CConfigJson::clear()
{
	SAFE_DELETE(m_pRoot);
}

CConfigObject* CConfigJson::getConfigObject(const char* strName) const
{
	CConfigObject	*pRoot, *pObj = 0;
	CConfigItem*	pItem;

	pRoot = dynamic_cast<CConfigObject*>(m_pRoot);
	if ( pRoot )  {
		if ( strName ) {
			pItem = (*pRoot)[strName];
			pObj = dynamic_cast<CConfigObject*>(pItem);
		}
		else {
			pObj = pRoot;
		}
	}

	return pObj;
}

CConfigArray* CConfigJson::getConfigArray(const char* strName) const
{
	CConfigObject*	pRoot;
	CConfigArray*	pArr = 0;
	CConfigItem*	pItem;

	if ( strName ) {
		pRoot = dynamic_cast<CConfigObject*>(m_pRoot);
		if ( pRoot ) {
			pItem = (*pRoot)[strName];
			pArr = dynamic_cast<CConfigArray*>(pItem);
		}
	}
	else {
		pArr = dynamic_cast<CConfigArray*>(m_pRoot);
	}

	return pArr;
}


/*
 * Load and build configuration tree from JSON file
 *
 * Return: ESUCCESS, ENOENT, ENOMEM, EINVAL, ...
 */
result_t CConfigJson::load()
{
	CJsonOA*		pRoot;
	result_t	nresult;

	if ( m_strFile.isEmpty() )  {
		log_debug(L_GEN, "[confbase] no configuration file specified\n");
		return EINVAL;
	}

	clear();

	pRoot = new CJsonObject;
	nresult = pRoot->loadFile(m_strFile);
	if ( nresult != ESUCCESS )  {
		delete pRoot;
		pRoot = new CJsonArray;
		nresult = pRoot->loadFile(m_strFile);
		if ( nresult != ESUCCESS )  {
			delete pRoot;

			log_error(L_GEN, "[confbase] failed to load file %s, result %d\n",
					  m_strFile.cs(), nresult);
			return nresult;
		}
	}

	if ( pRoot->isObject() )  {
		m_pRoot = new CConfigObject("/");
	}
	else {
		m_pRoot = new CConfigArray("/");
	}

	nresult = m_pRoot->load(pRoot);
	if ( nresult == ESUCCESS ) {
		m_pRoot->setModified(FALSE);
	}
	else {
		clear();
	}

	delete pRoot;

	return nresult;
}

result_t CConfigJson::save(int options)
{
	char		strTmpFile[256];
	result_t	nresult;

	if ( m_strFile.isEmpty() )  {
		log_debug(L_GEN, "[confbase] no configuration file specified\n");
		return EINVAL;
	}

	if ( options&JSON_OPTION_SAVE_SAFE )  {
		CFile	tmpFile;
		char*	p;

		copyString(strTmpFile, m_strFile, sizeof(strTmpFile));
		p = _tstrrchr(strTmpFile, PATH_SEPARATOR);
		if ( p )  {
			*p = '\0';
		}
		appendPath(strTmpFile, "tmpXXXXXX", sizeof(strTmpFile));

		nresult = tmpFile.createTempFile(strTmpFile);
		if ( nresult != ESUCCESS )  {
			log_error(L_GEN, "[confbase] can't create temp file '%s', result %d\n",
					  strTmpFile, nresult);
			return nresult;
		}

		copyString(strTmpFile, tmpFile.getName(), sizeof(strTmpFile));
		tmpFile.close();
	}

	if ( m_pRoot != 0 ) {
		CJsonOA*		pRoot;
		CString			strJson;

		if ( m_pRoot->getType() == CONFIG_ITEM_OBJECT ) {
			pRoot = new CJsonObject;
		}
		else {
			pRoot = new CJsonArray;
		}

		nresult = m_pRoot->save(pRoot);
		if ( nresult == ESUCCESS )  {
			nresult = pRoot->saveFile((options&JSON_OPTION_SAVE_SAFE) ?
									  strTmpFile : m_strFile, (json_option_t)options);
		}

		delete pRoot;
	}
	else {
		nresult = CFile::writeFile((options&JSON_OPTION_SAVE_SAFE) ?
								   strTmpFile : m_strFile, "{}\n", 3);
	}

	if ( options&JSON_OPTION_SAVE_SAFE )  {
		if ( nresult == ESUCCESS ) {
			nresult = CFile::renameFile(m_strFile, strTmpFile);
			if ( nresult != ESUCCESS )  {
				log_error(L_GEN, "[confbase] error rename file: %s => %s, result %d\n",
						  strTmpFile, m_strFile.cs(), nresult);
			}
		}

		if ( nresult != ESUCCESS ) {
			CFile::unlinkFile(strTmpFile);
		}
	}

	if ( nresult == ESUCCESS ) {
		setModified(FALSE);
	}
	else {
		log_debug(L_GEN, "[confbase] failed to save file %s, result %d\n",
				  m_strFile.cs(), nresult);
	}

	return nresult;
}

result_t CConfigJson::update(int options)
{
	result_t	nresult = ESUCCESS;

	if ( !m_pRoot || m_pRoot->isMofdified() )  {
		nresult = save(options);
	}

	return nresult;
}

/*******************************************************************************
 * Debugging support
 */

void CConfigJson::dump(const char* strPref) const
{
	if ( !m_pRoot )  {
		log_dump("*** CConfig: empty object\n");
		return;
	}

	log_dump("*** CConfig dump:\n");
	m_pRoot->dump(0, TRUE);

}
