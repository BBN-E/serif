// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "theories/EntityType.h"
#include "common/ParamReader.h"
#include "common/UnexpectedInputException.h"
#include "common/UTF8InputStream.h"

#include <iostream>
#include <cstring>
#include <boost/scoped_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/make_shared.hpp>

using namespace std;

EntityType::EntityTypes EntityType::_types = EntityType::EntityTypes();

EntityType::EntityType(Symbol name) {
	_info = getEntityTypeInfo(name);
}


EntityType EntityType::getPERType() {
	initEntityTypeInfo();
	if (_types.PER_index == ENTITY_TYPE_OTH_INDEX) {
		throw InternalInconsistencyException("EntityType::getPERType",
			"No PER type defined");
	}
	return EntityType(_types.PER_index);
}

EntityType EntityType::getORGType() {
	initEntityTypeInfo();
	if (_types.ORG_index == ENTITY_TYPE_OTH_INDEX) {
		throw InternalInconsistencyException("EntityType::getORGType",
			"No ORG type defined");
	}
	return EntityType(_types.ORG_index);
}

EntityType EntityType::getGPEType() {
	initEntityTypeInfo();
	if (_types.GPE_index == ENTITY_TYPE_OTH_INDEX) {
		throw InternalInconsistencyException("EntityType::getGPEType",
			"No GPE type defined");
	}
	return EntityType(_types.GPE_index);
}

EntityType EntityType::getLOCType() {
	initEntityTypeInfo();
	if (_types.LOC_index == ENTITY_TYPE_OTH_INDEX) {
		throw InternalInconsistencyException("EntityType::getLOCType",
			"No LOC type defined");
	}
	return EntityType(_types.LOC_index);
}

EntityType EntityType::getFACType() {
	initEntityTypeInfo();
	if (_types.FAC_index == ENTITY_TYPE_OTH_INDEX) {
		throw InternalInconsistencyException("EntityType::getFACType",
			"No FAC type defined");
	}
	return EntityType(_types.FAC_index);
}

EntityType EntityType::getNationalityType() {
	initEntityTypeInfo();
	if (_types.nationality_index == ENTITY_TYPE_OTH_INDEX) {
		throw InternalInconsistencyException("EntityType::getNationalityType",
			"No nationality type defined");
	}
	return EntityType(_types.nationality_index);
}

EntityType EntityType::getTempType() {
	initEntityTypeInfo();
	return EntityType(_types.temp_index);
}

EntityType EntityType::getABSType() {
	initEntityTypeInfo();
	if (_types.ABS_index == ENTITY_TYPE_OTH_INDEX) {
		throw InternalInconsistencyException("EntityType::getABSType",
			"No ABS type defined");
	}
	return EntityType(_types.ABS_index);
}

EntityType::EntityType(int index) {
	_info = getEntityTypeInfo(index);
}

bool EntityType::isValidEntityType(Symbol possibleType) {
	try {
		getEntityTypeInfo(possibleType);
		return true;
	} catch (...) {
		return false;
	}
}

const EntityType::EntityTypeInfo_ptr EntityType::getEntityTypeInfo(Symbol name) {
	// Make sure the type info is initialized
	initEntityTypeInfo();

	// Get this type if it's defined
	EntityTypeMap::const_iterator info_i = _types.typesByName.find(name);
	if (info_i != _types.typesByName.end()) {
		return getEntityTypeInfo((*info_i).second);
	} else if (ParamReader::isParamTrue("allow_dynamic_entity_types")) {
		return addEntityTypeInfo(name);
	} else {
		std::wstringstream error;
		error << "Unrecognized entity type: " << name;
		throw UnexpectedInputException("EntityType::getEntityTypeInfo()", error);
	}
}

const EntityType::EntityTypeInfo_ptr EntityType::getEntityTypeInfo(int index) {
	// Make sure the type info is initialized
	initEntityTypeInfo();

	// Get this type if it's defined
	if (index >= 0 && (size_t)index < _types.types.size()) {
		return _types.types[index];
	} else {
		throw InternalInconsistencyException::arrayIndexException(
			"EntityType::getEntityTypeInfo", _types.types.size(), index);
	}
}

const EntityType::EntityTypeInfo_ptr EntityType::addEntityTypeInfo(Symbol name) {
	// Create a new type with the specified name, and no options
	EntityTypeInfo_ptr info = boost::make_shared<EntityTypeInfo>();
	info->index = static_cast<int>(_types.types.size());
	info->name = name;
	info->idf_desc = false;
	info->rel_arg = false;
	info->linkable = true;
	info->is_temp = false;
	info->is_nationality = false;
	info->is_nestable = false;
	info->use_simple_coref = false;
	info->matches_PER = false;
	info->matches_ORG = false;
	info->matches_GPE = false;
	info->matches_FAC = false;
	info->matches_LOC = false;
	info->primary_tag = Symbol();
	info->secondary_tag = Symbol();
	info->default_tag = Symbol();
	info->default_word = Symbol();
	_types.types.push_back(info);
	_types.typesByName.insert(make_pair(info->name, info->index));
	return info;
}

void EntityType::initEntityTypeInfo() {
	// if the type info vector is not initialized, then initialize it
	if (_types.types.size() == 0) {
		// first put UNDET in
		if (_types.types.size() != ENTITY_TYPE_UNDET_INDEX) {
			std::wstringstream error;
			error << "UNDET type would have index " << _types.types.size() << " instead of " << ENTITY_TYPE_UNDET_INDEX;
			throw InternalInconsistencyException("EntityType::initEntityTypeInfo", error);
		}
		EntityTypeInfo_ptr undet = boost::make_shared<EntityTypeInfo>();
		undet->index = static_cast<int>(_types.types.size());
		undet->name = Symbol(L"UNDET");
		undet->idf_desc = false;
		undet->rel_arg = false;
		undet->linkable = true;
		undet->is_temp = false;
		undet->is_nationality = false;
		undet->is_nestable = false;
		undet->use_simple_coref = false;
		undet->matches_PER = false;
		undet->matches_ORG = false;
		undet->matches_GPE = false;
		undet->matches_FAC = false;
		undet->matches_LOC = false;
		undet->primary_tag = Symbol();
		undet->secondary_tag = Symbol();
		undet->default_tag = Symbol();
		undet->default_word = Symbol();
		_types.types.push_back(undet);
		_types.typesByName.insert(make_pair(undet->name, undet->index));

		// now put OTH in
		if (_types.types.size() != ENTITY_TYPE_OTH_INDEX) {
			std::wstringstream error;
			error << "OTH type would have index " << _types.types.size() << " instead of " << ENTITY_TYPE_OTH_INDEX;
			throw InternalInconsistencyException("EntityType::initEntityTypeInfo", error);
		}
		EntityTypeInfo_ptr oth = boost::make_shared<EntityTypeInfo>();
		oth->index = static_cast<int>(_types.types.size());
		oth->name = Symbol(L"OTH");
		oth->idf_desc = false;
		oth->rel_arg = false;
		oth->linkable = true;
		oth->is_temp = false;
		oth->is_nationality = false;
		oth->is_nestable = false;
		oth->use_simple_coref = false;
		oth->matches_PER = false;
		oth->matches_ORG = false;
		oth->matches_GPE = false;
		oth->matches_FAC = false;
		oth->matches_LOC = false;
		oth->primary_tag = Symbol();
		oth->secondary_tag = Symbol();
		oth->default_tag = Symbol();
		oth->default_word = Symbol();
		_types.types.push_back(oth);
		_types.typesByName.insert(make_pair(oth->name, oth->index));

		// now read in the real types from a file
		std::string file_name = ParamReader::getRequiredParam("entity_type_set");
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(file_name.c_str());
		if (in.fail()) {
			std::stringstream errMsg;
			errMsg << "Could not open entity type list:\n" << file_name << "\nSpecified by parameter entity_type_set";
			throw UnexpectedInputException(
				"EntityType::initEntityTypeInfo()",
				errMsg.str().c_str());
		}

		// Read into a wstringstream and immediately close the file.
		// Hopefully this helps with concurrent file access.
		std::vector<std::wstring> lines;
		while (in) {
			lines.push_back(std::wstring());
			std::getline(in, lines.back());
		}
		in.close();
		
		BOOST_FOREACH(std::wstring& line, lines) {
			size_t len = line.size();
			if (len && line[len-1] == L'\r') {
				line[len-1] = L'\0';
			}
			wchar_t *p = const_cast<wchar_t*>(line.c_str());

			// skip whitespace
			while ((*p == L'\t' || *p == L' ') && *p != L'\0') p++;
			if (*p == L'\0' || *p == L'#')
				continue;

			// populate element #i
			int i = static_cast<int>(_types.types.size());

			wchar_t *name = p;
			// skip over name
			while (*p != L'\t' && *p != L' ' && *p != L'\0') p++;
			// if there's more in the line, move pointer over to it
			if (*p != L'\0') {
				// but first set put in null terminator for name string
				*p = L'\0';
				p++;
			}

			EntityTypeInfo_ptr info = boost::make_shared<EntityTypeInfo>();
			info->index = i;
			info->name = Symbol(name);

			info->idf_desc =
				(wcsstr(const_cast<const wchar_t *>(p), L"-IdFDesc") != 0);
			info->rel_arg =
				(wcsstr(const_cast<const wchar_t *>(p), L"-NotRelArg") == 0);
			info->linkable =
				(wcsstr(const_cast<const wchar_t *>(p), L"-NoLink") == 0);
			info->matches_PER =
				(wcsstr(const_cast<const wchar_t *>(p), L"-MatchesPER") != 0);
			info->matches_ORG =
				(wcsstr(const_cast<const wchar_t *>(p), L"-MatchesORG") != 0);
			info->matches_GPE =
				(wcsstr(const_cast<const wchar_t *>(p), L"-MatchesGPE") != 0);
			info->matches_FAC =
				(wcsstr(const_cast<const wchar_t *>(p), L"-MatchesFAC") != 0);
			info->matches_LOC =
				(wcsstr(const_cast<const wchar_t *>(p), L"-MatchesLOC") != 0);
			info->is_nationality =
				(wcsstr(const_cast<const wchar_t *>(p), L"-IsNat") != 0);
			info->is_temp =
				(wcsstr(const_cast<const wchar_t *>(p), L"-IsTemp") != 0 ||
				 wcsstr(const_cast<const wchar_t *>(p), L"-IsDefTemp") != 0);
			info->is_nestable =
				(wcsstr(const_cast<const wchar_t *>(p), L"-IsNestable") != 0);
			info->use_simple_coref = 
				(wcsstr(const_cast<const wchar_t *>(p), L"-UseSimpleCoref") != 0);

#if defined(_WIN32)
			wchar_t *b = wcsstr(p, L"-PrimaryParseTag=");
#else
			wchar_t *b = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(p), L"-PrimaryParseTag="));
#endif
			if (b != 0) {
#if defined(_WIN32)
				wchar_t *tag = wcsstr(b, L"=") + 1;
#else
				wchar_t *tag = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(b), L"=")) + 1;
#endif
				b = tag;
				while (*b != L'\t' && *b != L' ' && *b != L'\0') b++;
				if (*b != L'\0') {
					wchar_t e = *b;
					*b = L'\0';
					info->primary_tag = Symbol(tag);
					*b = e;
				}
				else
					info->primary_tag = Symbol(tag);
			}
			else {
				info->primary_tag = Symbol();
			}

#if defined(_WIN32)
			b = wcsstr(p, L"-SecondaryParseTag=");
#else
			b = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(p), L"-SecondaryParseTag="));
#endif
			if (b != 0) {
#if defined(_WIN32)
				wchar_t *tag = wcsstr(b, L"=") + 1;
#else
				wchar_t *tag = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(b), L"=")) + 1;
#endif
				b = tag;
				while (*b != L'\t' && *b != L' ' && *b != L'\0') b++;
				if (*b != L'\0') {
					wchar_t e = *b;
					*b = L'\0';
					info->secondary_tag = Symbol(tag);
					*b = e;
				}
				else
					info->secondary_tag = Symbol(tag);
			}
			else {
				info->secondary_tag = Symbol();
			}

#if defined(_WIN32)
			b = wcsstr(p, L"-DefaultParseTag=");
#else
			b = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(p), L"-DefaultParseTag="));
#endif
			if (b != 0) {
#if defined(_WIN32)
				wchar_t *tag = wcsstr(b, L"=") + 1;
#else
				wchar_t *tag = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(b), L"=")) + 1;
#endif
				b = tag;
				while (*b != L'\t' && *b != L' ' && *b != L'\0') b++;
				if (*b != L'\0') {
					wchar_t e = *b;
					*b = L'\0';
					info->default_tag = Symbol(tag);
					*b = e;
				}
				else
					info->default_tag = Symbol(tag);
			}
			else {
				info->default_tag = Symbol();
			}

#if defined(_WIN32)
			b = wcsstr(p, L"-DefaultNameWord=");
#else
			b = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(p), L"-DefaultNameWord="));
#endif
			if (b != 0) {
#if defined(_WIN32)
				wchar_t *word = wcsstr(b, L"=") + 1;
#else
				wchar_t *word = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(b), L"=")) + 1;
#endif
				b = word;
				while (*b != L'\t' && *b != L' ' && *b != L'\0') b++;
				if (*b != L'\0') {
					wchar_t e = *b;
					*b = L'\0';
					info->default_word = Symbol(word);
					*b = e;
				}
				else
					info->default_word = Symbol(word);
			}
			else {
				info->default_word = Symbol();
			}

			if (_types.PER_index == ENTITY_TYPE_OTH_INDEX &&
				info->matches_PER)
			{
				_types.PER_index = i;
			}
			if (_types.ORG_index == ENTITY_TYPE_OTH_INDEX &&
				info->matches_ORG)
			{
				_types.ORG_index = i;
			}
			if (_types.GPE_index == ENTITY_TYPE_OTH_INDEX &&
				info->matches_GPE)
			{
				_types.GPE_index = i;
			}
			if (_types.LOC_index == ENTITY_TYPE_OTH_INDEX &&
				info->matches_LOC)
			{
				_types.LOC_index = i;
			}
			if (_types.FAC_index == ENTITY_TYPE_OTH_INDEX &&
				info->matches_FAC)
			{
				_types.FAC_index = i;
			}
			if (_types.nationality_index == ENTITY_TYPE_OTH_INDEX &&
				info->is_nationality)
			{
				_types.nationality_index = i;
			}
			if (_types.temp_index == ENTITY_TYPE_OTH_INDEX &&
				wcsstr(const_cast<const wchar_t *>(p), L"-IsDefTemp") != 0)
			{
				_types.temp_index = i;
			}

			_types.types.push_back(info);
			_types.typesByName.insert(make_pair(info->name, i));
		}
	}
}
