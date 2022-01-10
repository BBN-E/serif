// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "theories/EntitySubtype.h"
#include "common/ParamReader.h"
#include "common/UnexpectedInputException.h"
#include "common/UTF8InputStream.h"
#include "common/UTF8Token.h"

#include <iostream>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace std;

EntitySubtype::EntitySubtypes EntitySubtype::_subtypes = EntitySubtype::EntitySubtypes();

EntitySubtype::EntitySubtype(Symbol entityTypeName, Symbol entitySubtypeName) {
	_info = getEntitySubtypeInfo(joinEntityTypeAndSubtypeNames(entityTypeName, entitySubtypeName));
}

EntitySubtype::EntitySubtype(Symbol fullTypeName) {
	_info = getEntitySubtypeInfo(fullTypeName);
}

const EntitySubtype::EntitySubtypeInfo_ptr EntitySubtype::getEntitySubtypeInfo(Symbol fullTypeName) {
	// Make sure the subtype info is initialized
	initEntitySubtypeInfo();

	// Get this subtype if it's defined
	EntitySubtypeMap::const_iterator info_i = _subtypes.subtypesByFullName.find(fullTypeName);
	if (info_i != _subtypes.subtypesByFullName.end()) {
		return getEntitySubtypeInfo((*info_i).second);
	} else if (splitEntitySubtypeName(fullTypeName) == Symbol(L"UNDET")) {
		return getUndetType()._info;
	} else if (splitEntityTypeName(fullTypeName) == fullTypeName) {
		return getDefaultSubtype(EntityType(fullTypeName))._info;
	} else if (ParamReader::isParamTrue("allow_dynamic_entity_types")) {
		return addEntitySubtypeInfo(fullTypeName);
	} else {
		std::wstringstream error;
		error << "Unrecognized entity type and subtype: " << fullTypeName;
		throw UnexpectedInputException("EntitySubtype::getEntitySubtypeInfo()", error);
	}
}

const EntitySubtype::EntitySubtypeInfo_ptr EntitySubtype::getEntitySubtypeInfo(int index) {
	// Make sure the subtype info is initialized
	initEntitySubtypeInfo();

	// Get this type if it's defined
	if (index >= 0 && (size_t)index < _subtypes.subtypes.size()) {
		return _subtypes.subtypes[index];
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"EntitySubtype::getEntitySubtypeInfo", _subtypes.subtypes.size(), index);
	}
}

const EntitySubtype::EntitySubtypeInfo_ptr EntitySubtype::addEntitySubtypeInfo(Symbol fullTypeName) {
	// Create a new subtype with the specified name
	EntitySubtypeInfo_ptr info = boost::make_shared<EntitySubtypeInfo>();
	info->index = static_cast<int>(_subtypes.subtypes.size());
	info->name = splitEntitySubtypeName(fullTypeName);
	info->parentEntityType = EntityType(splitEntityTypeName(fullTypeName));
	_subtypes.subtypes.push_back(info);
	_subtypes.subtypesByFullName.insert(make_pair(fullTypeName, info->index));
	return info;
}

void EntitySubtype::initEntitySubtypeInfo() {
	// if the subtype info vector is not initialized, then initialize it
	if (_subtypes.subtypes.size() == 0) {
		// reserve space for default subtypes
		_subtypes.defaultSubtypes.resize(EntityType::getNTypes());

		// first put UNDET in
		if (_subtypes.subtypes.size() != ENTITY_TYPE_UNDET_INDEX) {
			std::wstringstream error;
			error << "UNDET subtype would have index " << _subtypes.subtypes.size() << " instead of " << ENTITY_TYPE_UNDET_INDEX;
			throw InternalInconsistencyException("EntitySubtype::initEntitySubtypeInfo", error);
		}
		EntitySubtypeInfo_ptr undet = boost::make_shared<EntitySubtypeInfo>();
		undet->index = static_cast<int>(_subtypes.subtypes.size());
		undet->name = Symbol(L"UNDET");
		undet->parentEntityType = EntityType::getUndetType();
		_subtypes.subtypes.push_back(undet);
		_subtypes.subtypesByFullName.insert(make_pair(joinEntityTypeAndSubtypeNames(undet->name, undet->parentEntityType.getName()), undet->index));

		// now optionally read in the real types from a file
		std::string file_name = ParamReader::getParam("entity_subtype_set");
		if (file_name.empty())
			return;
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(file_name.c_str());
		if (in.fail()) {
			std::stringstream errMsg;
			errMsg << "Could not open entity subtype list:\n" << file_name << "\nSpecified by parameter entity_subtype_set";
			throw UnexpectedInputException(
				"EntitySubtype::initEntitySubtypeInfo()",
				errMsg.str().c_str());
		}

		UTF8Token token;
		int index = 0;
		in >> token;
		while (!in.eof()) {
			if (wcscmp(token.chars(), L"") == 0)
				break;

			// populate element #i
			EntitySubtypeInfo_ptr info = addEntitySubtypeInfo(token.symValue());

			in >> token;
			if (wcscmp(token.chars(), L"") == 0)
				break;
			if (token.symValue() == Symbol(L"DEFAULT")) {
				int entityTypeIndex = info->parentEntityType.getNumber();
				if ((size_t)entityTypeIndex > _subtypes.defaultSubtypes.size())
					_subtypes.defaultSubtypes.resize(entityTypeIndex + 1);
				_subtypes.defaultSubtypes[entityTypeIndex] = info->index;
				in >> token;
			}
		}
	}
}

EntitySubtype EntitySubtype::getDefaultSubtype(EntityType entityType) {
	initEntitySubtypeInfo();
	if (entityType.getNumber() >= 0 && (size_t)entityType.getNumber() < _subtypes.defaultSubtypes.size()) {
		return EntitySubtype(_subtypes.defaultSubtypes[entityType.getNumber()]);
	} else {
		return getUndetType();
	}
}

EntitySubtype EntitySubtype::getSubtypeByIndex(int index) {
	initEntitySubtypeInfo();
	if (subtypesDefined()) {
		return EntitySubtype(index);
	} else {
		return getUndetType();
	}
}

bool EntitySubtype::isValidEntitySubtype(Symbol possibleSubtype) {
	try {
		getEntitySubtypeInfo(possibleSubtype);
		return true;
	} catch (...) {
		return false;
	}
}

bool EntitySubtype::isValidEntitySubtype(EntityType entityType, EntitySubtype entitySubtype) {
	return entitySubtype == EntitySubtype::getUndetType()  || 
		   entitySubtype.getParentEntityType() == entityType;
}

EntityType EntitySubtype::getParentEntityType(Symbol fullTypeName) {
	return EntityType(splitEntityTypeName(fullTypeName));
}

Symbol EntitySubtype::splitEntityTypeName(Symbol fullTypeName) {
	std::wstring str = fullTypeName.to_string();
	size_t index = str.find_last_of(L".");
	return Symbol(str.substr(0, index));
}

Symbol EntitySubtype::splitEntitySubtypeName(Symbol fullTypeName) {
	std::wstring str = fullTypeName.to_string();
	size_t index = str.find_last_of(L".");
	if (index == std::wstring::npos)
		return fullTypeName;
	else
		return Symbol(str.substr(index + 1));
}

Symbol EntitySubtype::joinEntityTypeAndSubtypeNames(Symbol entityTypeName, Symbol entitySubtypeName) {
	if (entityTypeName.is_null())
		throw InternalInconsistencyException("EntitySubtype::joinEntityTypeAndSubtypeNames", "Empty EntityType name");
	if (entitySubtypeName.is_null() || entityTypeName == entitySubtypeName)
		return entityTypeName + Symbol(L".UNDET");
	return entityTypeName + Symbol(L".") + entitySubtypeName;
}

EntitySubtype::EntitySubtype(int index) {
	_info = getEntitySubtypeInfo(index);
}
