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
 *  Revision 1.0, 11.04.2018 11:17:31
 *      Initial revision.
 *
 *  Revision 1.1, 19.10.2018 13:14:17
 *  	Added support of 'null' data type.
 */

#include "shell/file.h"
#include "carbon/memory.h"
#include "carbon/json.h"

/*
 * Convert internal library JSON type (JSON_xxx) to external (JSON_OBJ_xxx) type
 *
 * 		jansson_type		internal JSON type
 *
 * Return: external JSON type representation
 */
/*static json_item_type_t _convType(int jansson_type)
{
	json_item_type_t		type;

	switch(jansson_type) {
		case JSON_OBJECT: 	type = JSON_ITEM_OBJECT; 	break;
		case JSON_ARRAY: 	type = JSON_ITEM_ARRAY;		break;
		case JSON_STRING: 	type = JSON_ITEM_STRING; 	break;
		case JSON_INTEGER:	type = JSON_ITEM_INTEGER;	break;
		case JSON_REAL: 	type = JSON_ITEM_FLOAT;		break;
		case JSON_TRUE: 	type = JSON_ITEM_BOOLEAN;	break;
		case JSON_FALSE: 	type = JSON_ITEM_BOOLEAN;	break;
		case JSON_NULL: 	type = JSON_ITEM_NULL; 		break;
		default: 			type = JSON_ITEM_NULL; 		break;
	}

	return type;
}*/

/*
 * Make formatted error string
 *
 * 		pJsonError		error data
 * 		strBuffer		output buffer for formatted string
 * 		length			output buffer maximum length
 */
static void _formatError(json_error_t* pJsonError, char* strBuffer,
							  size_t length)
{
	_tsnprintf(strBuffer, length, "L:%d C:%d POS:%d SRC:%s %s",
			 pJsonError->line, pJsonError->column, pJsonError->position,
			 pJsonError->source, pJsonError->text);
}

/*******************************************************************************
 * CJsonItem class
 */

CJsonItem* CJsonItem::getRoot()
{
	CJsonItem		*pItem, *pParent;

	pItem = this;
	while ( (pParent=pItem->getParent()) != nullptr )  {
		pItem = pParent;
	}

	return pItem;
}

void CJsonItem::dumpIndent(int nIndent, char* strBuf, size_t maxLength) const
{
	int 	i, n = sh_min(nIndent, (int)maxLength);

	for(i=0; i<n; i++)  {
		strBuf[i] = ' ';
	}
	strBuf[n] = '\0';
}

/*******************************************************************************
 * CJsonString class
 */

result_t CJsonString::doLoad(json_t* pItem)
{
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);

	if ( json_is_string(pItem) )  {
		m_strValue = json_string_value(pItem);
	}
	else {
		log_error(L_GEN, "[json] can't load, expected STRING but found item type %d\n",
				  json_typeof(pItem));
		nresult = EINVAL;
	}

	return nresult;
}

result_t CJsonString::doSave(json_t* pItem) const
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);

	retVal = json_string_set(pItem, m_strValue);
	if ( retVal != 0 )  {
		log_error(L_GEN, "[json] failure to set JSON string value\n");
		nresult = EINVAL;
	}

	return nresult;
}

void CJsonString::dump(int nIndent, boolean_t bLast) const
{
	char			strBuf[256];
	size_t			l, length;
	boolean_t		bParentArray;

	bParentArray = (getParent() != 0 && getParent()->getType() == JSON_ITEM_ARRAY);

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

void CJsonString::dump(const char* strPref) const
{
	log_dump("%s\"%s\": \"%s\"\n", strPref, getName(), m_strValue.cs());
}


/*******************************************************************************
 * CJsonInteger class
 */

result_t CJsonInteger::doLoad(json_t* pItem)
{
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);

	if ( json_is_integer(pItem) )  {
		m_nValue = json_integer_value(pItem);
	}
	else {
		log_error(L_GEN, "[json] can't load, expected INTEGER but found item type %d\n",
				  json_typeof(pItem));
		nresult = EINVAL;
	}

	return nresult;
}

result_t CJsonInteger::doSave(json_t* pItem) const
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);

	retVal = json_integer_set(pItem, m_nValue);
	if ( retVal != 0 )  {
		log_error(L_GEN, "[json] failure to ser JSON integer value\n");
		nresult = EINVAL;
	}

	return nresult;
}

void CJsonInteger::dump(int nIndent, boolean_t bLast) const
{
	char			strBuf[256];
	size_t			l, length;
	boolean_t		bParentArray;

	bParentArray = (getParent() != 0 && getParent()->getType() == JSON_ITEM_ARRAY);

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

void CJsonInteger::dump(const char* strPref) const
{
	log_dump("%s\"%s\": %" PRIi64 "\n", strPref, getName(), m_nValue);
}


/*******************************************************************************
 * CJsonBoolean class
 */

result_t CJsonBoolean::doLoad(json_t* pItem)
{
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);

	if ( json_is_boolean(pItem) )  {
		m_nValue = json_boolean_value(pItem);
	}
	else {
		log_error(L_GEN, "[json] can't load, expected BOOLEAN but found item type %d\n",
				  json_typeof(pItem));
		nresult = EINVAL;
	}

	return nresult;
}

result_t CJsonBoolean::doSave(json_t* pItem) const
{
	shell_assert(pItem);
	return ESUCCESS;
}

void CJsonBoolean::dump(int nIndent, boolean_t bLast) const
{
	char			strBuf[256];
	size_t			l, length;
	boolean_t		bParentArray;

	bParentArray = (getParent() != 0 && getParent()->getType() == JSON_ITEM_ARRAY);

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	if ( !bParentArray )  {
		_tsnprintf(&strBuf[l], length-l, "\"%s\": %s%s\n",
				   getName(), m_nValue ? "true" : "false", bLast ? "" : ",");
	}
	else {
		_tsnprintf(&strBuf[l], length-1, "%s%s\n",
				   m_nValue ? "true" : "false", bLast ? "" : ",");
	}

	log_dump(strBuf);
}

void CJsonBoolean::dump(const char* strPref) const
{
	log_dump("%s\"%s\": \"%s\"\n", strPref, getName(), m_nValue ? "true" : "false");
}

/*******************************************************************************
 * CJsonFloat class
 */

result_t CJsonFloat::doLoad(json_t* pItem)
{
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);

	if ( json_is_real(pItem) )  {
		m_dValue = json_real_value(pItem);
	}
	else {
		log_error(L_GEN, "[json] can't load, expected REAL but found item type %d\n",
				  json_typeof(pItem));
		nresult = EINVAL;
	}

	return nresult;
}

result_t CJsonFloat::doSave(json_t* pItem) const
{
	int 		retVal;
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);

	retVal = json_real_set(pItem, m_dValue);
	if ( retVal != 0 )  {
		log_error(L_GEN, "[json] failure to ser JSON integer value\n");
		nresult = EINVAL;
	}

	return nresult;
}

void CJsonFloat::dump(int nIndent, boolean_t bLast) const
{
	char			strBuf[256];
	size_t			l, length;
	boolean_t		bParentArray;

	bParentArray = (getParent() != 0 && getParent()->getType() == JSON_ITEM_ARRAY);

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	if ( !bParentArray )  {
		_tsnprintf(&strBuf[l], length-l, "\"%s\": %f%s\n",
				   getName(), m_dValue, bLast ? "" : ",");
	}
	else {
		_tsnprintf(&strBuf[l], length-1, "%f%s\n",
				   m_dValue, bLast ? "" : ",");
	}

	log_dump(strBuf);
}

void CJsonFloat::dump(const char* strPref) const
{
	log_dump("%s\"%s\": %f\n", strPref, getName(), m_dValue);
}

/*******************************************************************************
 * CJsonNull class
 */

result_t CJsonNull::doLoad(json_t* pItem)
{
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);

	if ( !json_is_null(pItem) )  {
		log_error(L_GEN, "[json] can't load, expected REAL but found item type %d\n",
				  json_typeof(pItem));
		nresult = EINVAL;
	}

	return nresult;
}

result_t CJsonNull::doSave(json_t* pItem) const
{
	shell_assert(pItem);
	return ESUCCESS;
}

void CJsonNull::dump(int nIndent, boolean_t bLast) const
{
	char			strBuf[256];
	size_t			l, length;
	boolean_t		bParentArray;

	bParentArray = (getParent() != 0 && getParent()->getType() == JSON_ITEM_ARRAY);

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	if ( !bParentArray )  {
		_tsnprintf(&strBuf[l], length-l, "\"%s\": null%s\n", getName(), bLast ? "" : ",");
	}
	else {
		_tsnprintf(&strBuf[l], length-1, "null%s\n", bLast ? "" : ",");
	}

	log_dump(strBuf);
}

void CJsonNull::dump(const char* strPref) const
{
	log_dump("%s\"%s\": null\n", strPref, getName());
}


/*******************************************************************************
 * CJsonOA class
 */
void CJsonOA::clear()
{
	while ( !m_item.empty() )  {
		delete *m_item.begin();
		m_item.erase(m_item.begin());
	}
}

CJsonItem* CJsonOA::operator[](size_t index) const
{
	if ( index < m_item.size() ) {
		json_item_list_t::const_iterator	it;
		size_t	i;

		it = m_item.begin();
		for(i=0; i<index; i++)  {
			it++;
		}

		return *it;
	}

	return 0;
}

CJsonItem* CJsonOA::createItem(const char* strName, json_type type) const
{
	CJsonItem*		pItem = 0;
	const char*		sName = strName ? strName : "";

	switch ( type ) {
		case JSON_OBJECT:
			pItem = new CJsonObject(sName);
			break;

		case JSON_ARRAY:
			pItem = new CJsonArray(sName);
			break;

		case JSON_STRING:
			pItem = new CJsonString(sName);
			break;

		case JSON_INTEGER:
			pItem = new CJsonInteger(sName);
			break;

		case JSON_TRUE:
		case JSON_FALSE:
			pItem = new CJsonBoolean(sName);
			break;

		case JSON_REAL:
			pItem = new CJsonFloat(sName);
			break;

		case JSON_NULL:
			pItem = new CJsonNull(sName);
			break;

		default:
			log_error(L_GEN, "[json] name '%s': unsupported JSON type %d\n", sName, type);
			break;
	}

	return pItem;
}

json_t* CJsonOA::createJanssonItem(json_item_type_t type, boolean_t bValue) const
{
	json_t*		pItem = 0;

	switch ( type )  {
		case JSON_ITEM_OBJECT:
			pItem = json_object();
			break;

		case JSON_ITEM_ARRAY:
			pItem = json_array();
			break;

		case JSON_ITEM_STRING:
			pItem = json_string("");
			break;

		case JSON_ITEM_INTEGER:
			pItem = json_integer(0);
			break;

		case JSON_ITEM_FLOAT:
			pItem = json_real(0.0);
			break;

		case JSON_ITEM_BOOLEAN:
			pItem = json_boolean(bValue);
			break;

		case JSON_ITEM_NULL:
			pItem = json_null();
			break;

		default:
			log_error(L_GEN, "[json] unexpected JSON item type %d\n", type);
			break;
	}

	if ( pItem == 0 )  {
		log_error(L_GEN, "[json] can't create jansson item, type %d\n", type);
	}

	return pItem;
}

void CJsonOA::copy(const CJsonOA& jsonOA)
{
	CJsonItem	*pItem, *pNewItem;
	size_t		i, count;

	this->clear();

	count = jsonOA.size();
	for(i=0; i<count; i++)  {
		pItem = jsonOA[i];
		pNewItem = pItem->clone();
		this->insert(pNewItem);
	}
}

result_t CJsonOA::load(const char* strJson)
{
	const char*		s;
	size_t			nLength;
	result_t		nresult = ESUCCESS;

	s = strJson;
	if ( s )  {
		SKIP_CHARS(s, " \t");
	}

	if ( s != NULL && *s != '\0' ) {
		nLength = _tstrlen(s);
		nresult = load(s, nLength);
	}
	else {
		/* Empty JSON */
		clear();
	}

	return nresult;
}

result_t CJsonOA::loadFile(const char* strFilename, json_option_t options)
{
	json_t*			pRoot;
	json_error_t	error;
	result_t		nresult;

	shell_unused(options);
	shell_assert(strFilename);

	if ( CFile::fileExists(strFilename, R_OK) ) {
		pRoot = json_load_file(strFilename, 0, &error);
		if ( pRoot )  {
			int 	reqType = getType() == JSON_ITEM_OBJECT ? JSON_OBJECT : JSON_ARRAY;

			if ( json_typeof(pRoot) == reqType ) {
				clear();
				nresult = doLoad(pRoot);
				if ( nresult != ESUCCESS )  {
					clear();
				}
			}
			else {
				log_error(L_GEN, "[json] specified json is of type %d, expected type %d\n",
						  	json_typeof(pRoot), reqType);
				nresult = EINVAL;
			}

			json_decref(pRoot);
		}
		else {
			char 	strBuf[128];

			_formatError(&error, strBuf, sizeof(strBuf));
			log_error(L_GEN, "[json] failed to load json file %s: %s\n", strFilename, strBuf);
			nresult = EFAULT;
		}
	}
	else {
		log_debug(L_GEN, "[json] file '%s' is not exists or is not readable\n",
				  strFilename);
		nresult = ENOENT;
	}

	return nresult;
}

result_t CJsonOA::saveFile(const char* strFilename, json_option_t options) const
{
	json_t*		pRoot;
	result_t	nresult;

	shell_assert(strFilename);

	pRoot = getType() == JSON_ITEM_OBJECT ? json_object() : json_array();
	if ( pRoot )  {
		nresult = doSave(pRoot);
		if ( nresult == ESUCCESS )  {
			size_t	flags = (options&JSON_OPTION_PRETTY) ? JSON_INDENT(JSON_PRETTY_INDENT) : 0;
			int 	retVal;

			retVal = json_dump_file(pRoot, strFilename, flags);
			if ( retVal != 0 )  {
				log_error(L_GEN, "[json] failure saving file %s\n", strFilename);
				nresult = EIO;
			}
		}

		json_decref(pRoot);
	}
	else {
		log_error(L_GEN, "[json] can't create root item(jansson)\n");
		nresult = ENOMEM;
	}

	return nresult;
}


/*******************************************************************************
 * CJsonObject class
 */

CJsonArray* CJsonObject::getArray(const char* strName) const {
	return dynamic_cast<CJsonArray*>(find(strName));
}


void CJsonObject::insert(CJsonItem* pJsonItem)
{
	shell_assert(pJsonItem);

	remove(pJsonItem->getName());

	pJsonItem->m_pParent = this;
	m_item.push_back(pJsonItem);
}

CJsonArray& CJsonObject::insertArray(const char* strName)
{
	CJsonArray*	pItem = new CJsonArray(strName);

	insert(pItem);
	return *pItem;
}

result_t CJsonObject::remove(const char* strName)
{
	json_item_list_t::iterator	it;
	CString		sName(strName);
	result_t	nresult = ENOENT;

	shell_assert(strName);

	it = m_item.begin();
	while ( it != m_item.end() ) {
		if ( sName == (*it)->getName() )  {
			delete *it;
			m_item.erase(it);
			nresult = ESUCCESS;
			break;
		}

		it++;
	}

	return nresult;
}

CJsonItem* CJsonObject::get(const char* strName) const
{
	json_item_list_t::const_iterator	it;
	CJsonItem*		pItem = 0;
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

result_t CJsonObject::doLoad(json_t* pItem)
{
	const char*		strIterName;
	json_t*			pIterItem;
	CJsonItem*		pJsonItem;
	result_t		nresult = ESUCCESS;

	shell_assert(pItem);
	shell_assert(size() == 0);

	if ( !json_is_object(pItem) ) {
		return EINVAL;
	}

	json_object_foreach(pItem, strIterName, pIterItem) {
		pJsonItem = createItem(strIterName, json_typeof(pIterItem));
		if ( pJsonItem )  {
			nresult = pJsonItem->doLoad(pIterItem);
			if ( nresult == ESUCCESS )  {
				insert(pJsonItem);
			}
			else {
				delete pJsonItem;
			}
		}
		else {
			log_error(L_GEN, "[json] failure loading JSON, can't create item '%s'\n",
					  strIterName);
			nresult = EINVAL;
		}

		if ( nresult != ESUCCESS ) {
			break;
		}
	}

	return nresult;
}

result_t CJsonObject::load(const char* strJson, size_t nLength)
{
	const char*		s;
	size_t			nLen;
	json_t*			pRoot;
	json_error_t	error;
	result_t		nresult;

	s = strJson;
	nLen = nLength;
	if ( s != 0 )  {
		while ( nLen > 0 && _tstrchr(" \t", *s) != NULL )  {
			s++; nLen--;
		}
	}

	if ( nLen == 0 || s == 0 || *s == '\0' )  {
		/* Empty JSON */
		clear();
		return ESUCCESS;
	}

	pRoot = json_loadb(strJson, nLen, 0, &error);
	if ( !pRoot )  {
		char 	strBuf[128];

		_formatError(&error, strBuf, sizeof(strBuf));
		log_error(L_GEN, "[json] failed to load JSON string: %s\n", strBuf);
		return EINVAL;
	}

	if ( json_is_object(pRoot) ) {
		clear();

		nresult = doLoad(pRoot);
		if ( nresult != ESUCCESS )  {
			clear();
		}
	}
	else {
		log_error(L_GEN, "[json] can't load JSON, expected object, but found %d type (jansson)\n",
				  	json_typeof(pRoot));
		nresult = EINVAL;
	}

	json_decref(pRoot);
	return nresult;
}

result_t CJsonObject::doSave(json_t* pItem) const
{
	size_t		i, count;
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);
	shell_assert(json_object_size(pItem) == 0);

	if ( !json_is_object(pItem) )  {
		return EINVAL;
	}

	count = size();
	for(i=0; i<count; i++)  {
		CJsonItem*	pJsonItem = (*this)[i];
		json_t*		pIterItem;
		boolean_t	bValue = FALSE;
		int 		retVal;

		if ( pJsonItem->getType() == JSON_ITEM_BOOLEAN ) 	{
			bValue = *dynamic_cast<CJsonBoolean*>(pJsonItem);
		}

		pIterItem = createJanssonItem(pJsonItem->getType(), bValue);
		if ( pIterItem )  {
			retVal = json_object_set_new(pItem, pJsonItem->getName(), pIterItem);
			if ( retVal == 0 )  {
				nresult = pJsonItem->doSave(pIterItem);
			}
			else {
				json_decref(pIterItem);
				log_error(L_GEN, "[json] failure to save item '%s'\n", pJsonItem->getName());
				nresult = EINVAL;
			}
		}
		else {
			nresult = EINVAL;
		}

		if ( nresult != ESUCCESS )  {
			break;
		}
	}

	return nresult;
}

result_t CJsonObject::save(CString& strJson, json_option_t options) const
{
	json_t*		pRoot;
	result_t	nresult;

	pRoot = json_object();
	if ( pRoot )  {
		nresult = doSave(pRoot);
		if ( nresult == ESUCCESS )  {
			size_t			flags = 0;
			const char*		s;

			if ( options&JSON_OPTION_PRETTY )  {
				flags |= JSON_INDENT(JSON_PRETTY_INDENT);
			} else if ( options&JSON_OPTION_COMPACT )  {
				flags |= JSON_COMPACT;
			}

			s = json_dumps(pRoot, flags);
			if ( s )  {
				strJson = s;
				memFree((void*)s);
			}
			else {
				log_error(L_GEN, "[json] failed to export JSON to string\n");
				nresult = ENOMEM;
			}
		}
		else {
			log_error(L_GEN, "[json] failure saving JSON, result %d\n", nresult);
		}

		json_decref(pRoot);
	}
	else {
		log_error(L_GEN, "[json] failure to create root JSON object\n");
		nresult = ENOMEM;
	}

	return nresult;
}

void CJsonObject::dump(int nIndent, boolean_t bLast) const
{
	json_item_list_t::const_iterator it;
	char				strBuf[256];
	size_t				l, length;
	const CJsonItem		*pItem, *pParent;
	boolean_t 			hasName;

	hasName = CString(getName()) != "";
	if ( hasName ) {
		pParent = getParent();
		hasName = pParent != 0 && pParent->getType() != JSON_ITEM_ARRAY;
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
		pItem->dump(nIndent + JSON_PRETTY_INDENT, it == m_item.end());
	}

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	_tsnprintf(&strBuf[l], length-l, "}%s\n", bLast ? "" : ",");
	log_dump(strBuf);
}

void CJsonObject::dump(const char* strPref) const
{
	log_dump("*** JSON Object dump%s:\n", strPref);
	dump(0, TRUE);
}


/*******************************************************************************
 * CJsonArray class
 */

CJsonItem* CJsonArray::insert(CJsonItem* pJsonItem, size_t index)
{
	size_t		length = size(), rindex;
	CJsonItem*	x = 0;

	rindex = index == (size_t)-1 ? length : index;
	if ( rindex < length )  {
		json_item_list_t::iterator	it;
		size_t	i;

		it = m_item.begin();
		for(i=0; i<rindex; i++)  {
			it++;
		}

		pJsonItem->m_pParent = this;
		m_item.insert(it, pJsonItem);
		x = pJsonItem;
	}
	else if ( rindex == length ) {
		pJsonItem->m_pParent = this;
		m_item.push_back(pJsonItem);
		x = pJsonItem;
	}
	else {
		log_error(L_GEN, "[json] can't insert array item, index=%lu, size=%lu\n",
				  	rindex, length);
	}

	return x;
}

result_t CJsonArray::remove(size_t index)
{
	result_t	nresult = EINVAL;

	if ( index < size() ) {
		json_item_list_t::iterator	it;
		size_t	i;

		it = m_item.begin();
		for (i = 0; i < index; i++) {
			it++;
		}

		delete *it;
		m_item.erase(it);
		nresult = ESUCCESS;
	}

	return nresult;
}

result_t CJsonArray::doLoad(json_t* pItem)
{
	size_t			nIndex;
	json_t*			pIterItem;
	CJsonItem*		pJsonItem;
	result_t		nresult = ESUCCESS;

	shell_assert(pItem);
	shell_assert(size() == 0);

	if ( !json_is_array(pItem) ) {
		return EINVAL;
	}

	json_array_foreach(pItem, nIndex, pIterItem) {
		pJsonItem = createItem("", json_typeof(pIterItem));
		if ( pJsonItem )  {
			nresult = pJsonItem->doLoad(pIterItem);
			if ( nresult == ESUCCESS )  {
				insert(pJsonItem);
			}
			else {
				delete pJsonItem;
			}
		}
		else {
			log_error(L_GEN, "[json] failure loading JSON, can't create array item #%lu\n",
					  nIndex);
			nresult = EINVAL;
		}

		if ( nresult != ESUCCESS ) {
			break;
		}
	}

	return nresult;
}

result_t CJsonArray::load(const char* strJson, size_t nLength)
{
	const char*		s;
	size_t			nLen;
	json_t*			pRoot;
	json_error_t	error;
	result_t		nresult;

	s = strJson;
	nLen = nLength;
	if ( s != 0 )  {
		while ( nLen > 0 && _tstrchr(" \t", *s) != NULL )  {
			s++; nLen--;
		}
	}

	if ( s == 0 || *s == '\0' || nLen == 0 )  {
		/* Empty JSON */
		clear();
		return ESUCCESS;
	}

	pRoot = json_loadb(strJson, nLen, 0, &error);
	if ( !pRoot )  {
		char 	strBuf[128];

		_formatError(&error, strBuf, sizeof(strBuf));
		log_error(L_GEN, "[json] failed to load JSON string: %s\n", strBuf);
		return EINVAL;
	}

	if ( json_is_array(pRoot) ) {
		clear();

		nresult = doLoad(pRoot);
		if ( nresult != ESUCCESS )  {
			clear();
		}
	}
	else {
		log_error(L_GEN, "[json] can't load JSON, expected array, but found %d type (jansson)\n",
				  json_typeof(pRoot));
		nresult = EINVAL;
	}

	json_decref(pRoot);
	return nresult;
}

result_t CJsonArray::doSave(json_t* pItem) const
{
	size_t		i, count;
	result_t	nresult = ESUCCESS;

	shell_assert(pItem);
	shell_assert(json_array_size(pItem) == 0);

	if ( !json_is_array(pItem) )  {
		return EINVAL;
	}

	count = size();
	for(i=0; i<count; i++)  {
		CJsonItem*	pJsonItem = (*this)[i];
		json_t*		pIterItem;
		boolean_t	bValue = FALSE;
		int 		retVal;

		if ( pJsonItem->getType() == JSON_ITEM_BOOLEAN ) 	{
			bValue = *dynamic_cast<CJsonBoolean*>(pJsonItem);
		}

		pIterItem = createJanssonItem(pJsonItem->getType(), bValue);
		if ( pIterItem )  {
			retVal = json_array_append_new(pItem, pIterItem);
			if ( retVal == 0 )  {
				nresult = pJsonItem->doSave(pIterItem);
			}
			else {
				json_decref(pIterItem);
				log_error(L_GEN, "[json] failure to save item #%lu\n", i);
				nresult = EINVAL;
			}
		}
		else {
			nresult = EINVAL;
		}

		if ( nresult != ESUCCESS )  {
			break;
		}
	}

	return nresult;
}

result_t CJsonArray::save(CString& strJson, json_option_t options) const
{
	json_t*		pRoot;
	result_t	nresult;

	pRoot = json_array();
	if ( pRoot )  {
		nresult = doSave(pRoot);
		if ( nresult == ESUCCESS )  {
			size_t			flags = 0;
			const char*		s;

			if ( options&JSON_OPTION_PRETTY )  {
				flags |= JSON_INDENT(JSON_PRETTY_INDENT);
			} else if ( options&JSON_OPTION_COMPACT )  {
				flags |= JSON_COMPACT;
			}

			s = json_dumps(pRoot, flags);
			if ( s )  {
				strJson = s;
				memFree((void*)s);
			}
			else {
				log_error(L_GEN, "[json] failed to export JSON to string\n");
				nresult = ENOMEM;
			}
		}
		else {
			log_error(L_GEN, "[json] failure saving JSON, result %d\n", nresult);
		}

		json_decref(pRoot);
	}
	else {
		log_error(L_GEN, "[json] failure to create root JSON array\n");
		nresult = ENOMEM;
	}

	return nresult;
}

void CJsonArray::dump(int nIndent, boolean_t bLast) const
{
	json_item_list_t::const_iterator it;
	char			strBuf[256];
	size_t			l, length;
	CJsonItem*		pItem;

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	if ( CString(getName()) != "" ) {
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
		pItem->dump(nIndent + JSON_PRETTY_INDENT, it == m_item.end());
	}

	length = sizeof(strBuf)-1;
	dumpIndent(nIndent, strBuf, length);
	l = _tstrlen(strBuf);

	_tsnprintf(&strBuf[l], length-l, "]%s\n", bLast ? "" : ",");
	log_dump(strBuf);
}

void CJsonArray::dump(const char* strPref) const
{
	log_dump("*** JSON Array dump%s:\n", strPref);
	dump(0, TRUE);
}
