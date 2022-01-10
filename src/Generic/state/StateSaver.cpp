// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/state/StateSaver.h"
#include "dynamic_includes/common/SerifRestrictions.h"
#include "Generic/state/ObjectIDTable.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/OutputUtil.h"

#include <sstream>
#include <wchar.h>

#define MIN_REAL -99999999
#define MAX_REAL 99999999

#ifdef _WIN32
	#define swprintf _snwprintf
#endif

// Current version: v1.14
const std::pair<int, int> StateSaver::_version = std::make_pair(1, 14);

StateSaver::StateSaver(const char *file_name, bool binary) {
	initialize(file_name, binary);
}

StateSaver::StateSaver(const wchar_t *file_name, bool binary) {
	std::string file_name_as_string = OutputUtil::convertToUTF8BitString(file_name);
	initialize(file_name_as_string.c_str(), binary);
}

/* Binary defaults to false, unless binary-state-files param is set. */
StateSaver::StateSaver(const char *file_name) {
	initialize(file_name, ParamReader::isParamTrue("binary_state_files"));
}

/* Binary defaults to false, unless binary-state-files param is set. */
StateSaver::StateSaver(const wchar_t *file_name) {
	std::string file_name_as_string = OutputUtil::convertToUTF8BitString(file_name);
	initialize(file_name_as_string.c_str(), ParamReader::isParamTrue("binary_state_files"));
}

StateSaver::StateSaver(OutputStream& text_out)
	: _binary(false), _depth(0), _text_out(&text_out),
	  _this_object_created_text_out(false) {}

void StateSaver::initialize(const char *file_name, bool binary) {
    _binary = binary;
	_depth = 0;
    if (_binary) {
        _bin_out.open(file_name, std::ios::binary);
		_text_out = 0;
		_this_object_created_text_out = false;
    } else {
        _text_out = _new UTF8OutputStream(file_name);
		_this_object_created_text_out = true;
    }
}

StateSaver::~StateSaver() {
    if (_binary) {
        _bin_out.close();
    } else if (_this_object_created_text_out) {
        _text_out->close();
		delete _text_out;
    }
}

void StateSaver::beginStateTree(const wchar_t *state_description) {
	if (_depth != 0) {
		throw InternalInconsistencyException("StateSaver::beginStateTree()",
			"Attempt to begin state tree at depth other than 0");
	}

	#ifdef BLOCK_FULL_SERIF_OUTPUT
	
	throw UnexpectedInputException("StateSaver::beginStateTree", "State saving not supported");

	#endif

	_depth++;
    if (!_binary) {
        (*_text_out) << L"(";
        _current_list_empty_so_far = true;
        _current_list_multiline = false;
    }

	saveWord(L"Serif-state");
	std::wstringstream version;
	version << L"v" << _version.first << L"." << _version.second;
	saveWord(version.str().c_str());
	saveString(state_description);
	saveInteger(ObjectIDTable::getSize());
}

void StateSaver::endStateTree() {
	if (_depth != 1) {
		throw InternalInconsistencyException("StateSaver::endStateTree()",
			"Attempt to end state tree at depth other than 1");
	}
	_depth--;
    if (!_binary) {
        (*_text_out) << L"\n)\n";
        _text_out->flush();
    }
}

void StateSaver::saveWord(const wchar_t *str) {
    if (_binary) {
        size_t length = wcslen(str) * 2; // Each wide char is two bytes
        _bin_out.write((char*) &length, sizeof(unsigned short int));
        _bin_out.write((char*) str, (std::streamsize) length);  // Note that we do not write out the null terminator
    } else {
        if (_current_list_empty_so_far) {
            (*_text_out) << str;
            _current_list_empty_so_far = false;
        } else {
            if (_current_list_multiline) {
                advanceToNextLine();
            } else {
                (*_text_out) << L" ";
            }
            (*_text_out) << str;
        }
    }    
}

void StateSaver::saveString(const wchar_t *str) {
    if (_binary) {
        size_t length = wcslen(str) * 2; // Each wide char is two bytes
        _bin_out.write((char*) &length, sizeof(unsigned short int));
        _bin_out.write((char*) str, (std::streamsize) length);  // Note that we do not write out the null terminator
    } else {

        // we need to temporarily modify the string to get the quotes out
        wchar_t *writable = const_cast<wchar_t *>(str);
        for (wchar_t *p = writable; *p != L'\0'; p++) {
            if (*p == L'"')
                *p = L'\xfffe';
        }
        if (_current_list_empty_so_far) {
            (*_text_out) << L"\"" << str << L"\"";
            _current_list_empty_so_far = false;
        } else {
            if (_current_list_multiline) {
                advanceToNextLine();
            } else {
                (*_text_out) << L" ";
            }
            (*_text_out) << L"\"" << str << L"\"";
        }

        // now restore string to have quotes again
        for (wchar_t *q = writable; *q != L'\0'; q++) {
            if (*q == L'\xfffe') {
                *q = L'"';
            }
        }
    }
}

void StateSaver::saveSymbol(Symbol symbol) {
    if (symbol.is_null()) {
        saveString(L"<null-symbol>");
    } else {
		saveString(symbol.to_string());
    }
}

void StateSaver::saveInteger(int integer) {
    if (_binary) {
        _bin_out.write((char*) &integer, sizeof(int));
    } else {
        wchar_t buf[12];
        swprintf(buf, 12, L"%d", integer);
        saveWord(buf);
    }
}

void StateSaver::saveUnsigned(unsigned x) {
    if (_binary) {
        _bin_out.write((char*) &x, sizeof(unsigned));
    } else {	
        wchar_t buf[12];
        swprintf(buf, 12, L"%d", x);
        saveWord(buf);
    }
}

void StateSaver::saveReal(float real) {
    if (_binary) {
        _bin_out.write((char*) &real, sizeof(float));
    } else {
        wchar_t buf[100];
        swprintf(buf, 100, L"%f", real);
	
        // Workaround: If a division by 0 or a log(0) occurs in the code, real can 
        // have this 1.#INF00 value, which can't be read back in.
        if (!wcscmp(buf, L"1.#INF00")) {
            swprintf(buf, 100, L"%f", MAX_REAL);
        }
        if (!wcscmp(buf, L"-1.#INF00")) {
            swprintf(buf, 100, L"%f", MIN_REAL);
        }
        saveWord(buf);
    }
}

void StateSaver::savePointer(const void *pointer) {
    if (_binary) {
        saveInteger(ObjectIDTable::getID(pointer));
    } else {
        wchar_t buf[13];
        buf[0] = L'@';
        swprintf(buf+1, 12, L"%d", ObjectIDTable::getID(pointer));
        saveWord(buf);
    }
}

void StateSaver::beginList(const wchar_t *name, const void *pointer) {
    if (_binary) {
        if (name != 0) {
            saveString(name);
            if (pointer != 0) {
                saveInteger(ObjectIDTable::getID(pointer));
            } else {
                saveInteger(-1);  // This is what is returned by loadList() if we have no pointer
            }
        }
    } else {
        advanceToNextLine();
        (*_text_out) << L"(";	
        _current_list_empty_so_far = true;
        _current_list_multiline = false;
        if (name != 0) {
            (*_text_out) << name;
            if (pointer != 0) {
                wchar_t buf[13];
                buf[0] = L'@';
                swprintf(buf+1, 12, L"%d", ObjectIDTable::getID(pointer));
                (*_text_out) << buf;
            }
            _current_list_empty_so_far = false;
        }
    }
    _depth++;
}

void StateSaver::endList() {
	if (_depth == 0) {
		throw InternalInconsistencyException("StateSaver::endList()",
			"Depth sunk below 0 -- there must have been more endList()s than\n"
			"beginList()s.");
	}
	_depth--;
    if (!_binary) {
        (*_text_out) << L")";
        _current_list_empty_so_far = false;
        _current_list_multiline = true;
#ifdef _DEBUG
        _text_out->flush();
#endif
    }
}


void StateSaver::advanceToNextLine() {
	(*_text_out) << L"\n";
    for (int i = 0; i < _depth; i++) {
		(*_text_out) << L"  ";
    }
}
