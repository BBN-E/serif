// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/theories/ValueType.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/UTF8InputStream.h"

#include <cwchar>
#include <iostream>
#include <boost/scoped_ptr.hpp>

using namespace std;

Symbol ValueType::NONE = Symbol(L"NONE");

ValueType::ValueType(Symbol name) : _info(0) {
	const ValueTypeArray &types = getValueTypeArray();

	// this ought to be a hashtable lookup instead of a walking a list, but
	// for our little value-type lists, it doesn't really matter
	for (int i = 0; i < (int)types.valueTypes.size(); i++) {
		if (types.valueTypes[i].name == name || types.valueTypes[i].nickname == name) {
			_info = &types.valueTypes[i];
			break;
		}
	}
	if (_info == 0) {
		char message[100];
		strcpy(message, "Unrecognized value type: ");
		strncat(message, name.to_debug_string(), 50);
		throw UnexpectedInputException("ValueType::ValueType()",
			message);
	}
}


ValueType::ValueType(int index) {
	const ValueTypeArray &types = getValueTypeArray();
	if ((unsigned) index >= (unsigned) types.valueTypes.size()) {
		throw InternalInconsistencyException::arrayIndexException(
			"ValueType::ValueType(int)", types.valueTypes.size(), index);
	}
	_info = &types.valueTypes[index];
}

Symbol ValueType::getBaseTypeSymbol() const {
	std::wstring str = _info->name.to_string();
	size_t index = str.find(L".");
	if (index >= str.length())
		return _info->name;
	return Symbol(str.substr(0, index).c_str());
}


Symbol ValueType::getSubtypeSymbol() const {
	std::wstring str = _info->name.to_string();
	size_t index = str.find(L".");
	if (index >= str.length())
		return NONE;
	// e.g. "DISC."
	if (index == str.length() - 1)
		return NONE;
	else return Symbol(str.substr(index + 1).c_str());
}

bool ValueType::isValidValueType(Symbol possibleType) {
	const ValueTypeArray &types = getValueTypeArray();

	for (int i = 0; i < (int)types.valueTypes.size(); i++) {
		if (types.valueTypes[i].name == possibleType ||
			types.valueTypes[i].nickname == possibleType)
		{
			return true;
		}
	}
	return false;
}

const ValueType::ValueTypeArray &ValueType::getValueTypeArray() {
	static ValueTypeArray array;
    ValueTypeInfo empty;
	// if the array is not initialized, then initialize it
	if (array.valueTypes.size() == 0) {
        array.valueTypes.push_back(empty);
		// first put UNDET in
		array.valueTypes[VALUE_TYPE_UNDET_INDEX].index =
			VALUE_TYPE_UNDET_INDEX;
		array.valueTypes[VALUE_TYPE_UNDET_INDEX].name = Symbol(L"UNDET");
		array.valueTypes[VALUE_TYPE_UNDET_INDEX].nickname = Symbol(L"UNDET");
		array.valueTypes[VALUE_TYPE_UNDET_INDEX].for_events_only = false;

		// now read in the real types from a file
		std::string file_name = ParamReader::getRequiredParam("value_type_set");
		boost::scoped_ptr<UTF8InputStream> in_scoped_ptr(UTF8InputStream::build());
		UTF8InputStream& in(*in_scoped_ptr);
		in.open(file_name.c_str());

		if (in.fail()) {
			std::stringstream errMsg;	
			errMsg << "Could not open value type list:\n" << file_name << "\nSpecified by parameter value_type_set";
			throw UnexpectedInputException("ValueType::getValueTypeArray()", errMsg.str().c_str());
		}

		wchar_t line[200];
		while (!in.eof()) {
			in.getLine(line, 200);
			wchar_t *p = line;

			// skip whitespace
			while ((*p == L'\t' || *p == L' ') && *p != L'\0') p++;
			if (*p == L'\0' || *p == L'#')
				continue;

			// populate element #i
			size_t i = array.valueTypes.size();

			wchar_t *name = p;
			// skip over name
			while (*p != L'\t' && *p != L' ' && *p != L'\0') p++;
			// if there's more in the line, move pointer over to it
			if (*p != L'\0') {
				// but first set put in null terminator for name string
				*p = L'\0';
				p++;
			}
            array.valueTypes.push_back(empty);
			array.valueTypes[i].index = static_cast<int>(i);
			array.valueTypes[i].name = Symbol(name);

			array.valueTypes[i].for_events_only=
				(wcsstr((const wchar_t *)p, L"-forEventsOnly") != 0);

#if defined(_WIN32)
			wchar_t *b = wcsstr(p, L"-nickname=");
#else
			wchar_t *b = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(p), L"-nickname="));
#endif
			if (b != 0) {
#if defined(_WIN32)
				wchar_t *nickname = wcsstr(b, L"=") + 1;
#else
				wchar_t *nickname = const_cast<wchar_t *>(wcsstr(const_cast<const wchar_t *>(b), L"=")) + 1;
#endif
				b = nickname;
				while (*b != L'\t' && *b != L' ' && *b != L'\0') b++;
				if (*b != L'\0') {
					wchar_t e = *b;
					*b = L'\0';
					array.valueTypes[i].nickname = Symbol(nickname);
					*b = e;
				}
				else
					array.valueTypes[i].nickname = Symbol(nickname);
			}
			else {
				array.valueTypes[i].nickname = array.valueTypes[i].name;
			}
		}
	}

	return array;
}

