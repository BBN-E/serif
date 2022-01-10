// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/state/StateLoader.h"
#include "Generic/state/StateSaver.h"
#include "Generic/state/ObjectPointerTable.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/HeapChecker.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/theories/Parse.h"

#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include <iostream>

unsigned int StateLoader::IntegerCompressionStart = 79879; //Initial token integer replacement
unsigned int StateLoader::IntegerCompressionTokenCount = 8; //How many of the most common tokens to replace

StateLoader::StateLoader(const char *file_name, bool binary) {
	initialize(file_name, binary);
}

StateLoader::StateLoader(const wchar_t *file_name, bool binary) {
	std::string file_name_as_string = OutputUtil::convertToUTF8BitString(file_name);
	initialize(file_name_as_string.c_str(), binary);
}

/* Binary defaults to false, unless binary-state-files param is set. */
StateLoader::StateLoader(const char *file_name) {
	initialize(file_name, ParamReader::isParamTrue("binary_state_files"));
}
StateLoader::StateLoader(const wchar_t *file_name) {
	std::string file_name_as_string = OutputUtil::convertToUTF8BitString(file_name);
	initialize(file_name_as_string.c_str(), ParamReader::isParamTrue("binary_state_files"));
}

void StateLoader::initialize(const char *file_name, bool binary) {
	_lineno = 0;
    _binary = binary;
	_use_compressed_state = _binary && ParamReader::isParamTrue("use_state_file_integer_compression");
    _this_object_opened_the_input_stream = true;
    if (_binary) {
		//std::cerr << "StateLoader from file name binary: " << file_name << std::endl;
        _bin_in = _new std::ifstream(file_name, std::ios::binary);
		_text_in = 0;
		if (_bin_in->fail()) {
			std::ostringstream err;
			err << "Unable to load state file: " << file_name;
			throw UnexpectedInputException("StateLoader::initialize", err.str().c_str());
		}
    } else {
		//std::cerr << "StateLoader from file name text: " << file_name << std::endl;
		_text_in = UTF8InputStream::build(file_name);
		_bin_in = 0;
		if (_text_in->fail()) {
			std::ostringstream err;
			err << "Unable to load state file: " << file_name;
			throw UnexpectedInputException("StateLoader::initialize", err.str().c_str());
		}
    }
	_stateByteBuffer = NULL;
}

StateLoader::StateLoader(std::wistream& in) {
	//std::cerr << "StateLoader from wide stream reference" << std::endl;
	_lineno = 0;
    _binary = false;
	_use_compressed_state = false;
    _this_object_opened_the_input_stream = false;
    _text_in = &in;
	_stateByteBuffer = NULL;
}

StateLoader::StateLoader(std::istream& in) {
	//std::cerr << "StateLoader from narrow stream reference" << std::endl;
	_lineno = 0;
    _binary = true;
	_use_compressed_state = ParamReader::isParamTrue("use_state_file_integer_compression");
    _this_object_opened_the_input_stream = false;
    _bin_in = &in;
	_stateByteBuffer = NULL;
}

StateLoader::StateLoader(ByteBuffer* byteBuffer) {
	//std::cerr << "StateLoader from byte pointer " << byteBuffer << std::endl;
	_lineno = 0;
    _binary = true;
	_use_compressed_state = ParamReader::isParamTrue("use_state_file_integer_compression");
    _this_object_opened_the_input_stream = false;
	_stateByteBuffer = byteBuffer;
}

StateLoader::~StateLoader() {
    if (_this_object_opened_the_input_stream) {
        if (_binary) {
            delete _bin_in;        
        } else {
            delete _text_in;
        }
    }
}

void StateLoader::_parse_version(const std::wstring &version_str) {
	std::wstringstream vstream(version_str);
	if (vstream.get() == L'v') {
		vstream >> _version.first;
		if (vstream.get() == L'.') {
			vstream >> _version.second;
			if (!vstream.fail() && vstream.eof())
				return;
		}
	}
	throw UnexpectedInputException("StateLoader::_parse_version",
		"Bad version string!");
}

void StateLoader::beginStateTree(const wchar_t *state_description) {
    if (_binary){
        ensureToken(L"Serif-state");
    } else {
        ensureToken(L"(");
        ensureToken(L"Serif-state");
	}

	// Read the version number.
	std::wstring versionStr = loadString();
	_parse_version(versionStr);
	if (_version > StateSaver::getVersion()) {
		SessionLogger::warn("state_loader") << "state file was written by a newer version "
			<< "of SERIF, and may not be backwards-compatible." << std::endl;
	}
    ensureToken(state_description, "Expected state tree for %s, but got: `%s'");	
    int n_objects = loadInteger();
    getObjectPointerTable().initialize(n_objects);
	clearParseRegistry();

	// Here's where we would load REAL_MAX_SENT_MENTIONS
}

void StateLoader::endStateTree() {
    if (!_binary) {
        ensureToken(L")");
    }
}

wchar_t *StateLoader::loadString() {
    if (_binary) {
        unsigned short int length;

		if (STATE_LOADER_USE_BYTE_BUFFER) {
			length = _stateByteBuffer->getShort();
			_stateByteBuffer->getBytes((unsigned char*)token_string, length);
		} else {
			_bin_in->read((char *)(&length), sizeof(unsigned short int));
			_bin_in->read((char*)token_string, length);
		}
        token_string[length/2] = 0;
        return token_string;
    } else {
        readNextToken();
        return token_string;
    }
}

void StateLoader::loadSpecificString(const wchar_t *str, const char *message) {
	char new_message[1000];
	sprintf(new_message, "%s: Expected specific string %%s but got: `%%s'", message);
	ensureToken(str, new_message);
}

Symbol StateLoader::loadSymbol() {
    if (_binary) {
        wchar_t* string = loadString();
        if (wcscmp(string, L"<null-symbol>")) {
            return Symbol(string);
        } else {
            return Symbol();
        }
    } else {
        readNextToken();
        if (wcscmp(token_string, L"<null-symbol>")) {
            return Symbol(token_string);
        } else {
            return Symbol();
        }
    }
}

int StateLoader::loadInteger() {
    if (_binary) {
        int i;

		if (STATE_LOADER_USE_BYTE_BUFFER) {
			i = _stateByteBuffer->getInt();
		} else {
			_bin_in->read((char *)(&i), sizeof(int));
		}
        return i;
    } else {
        readNextToken();
        return _wtoi(token_string);
    }
}
unsigned StateLoader::loadUnsigned() {
    if (_binary) {
        unsigned u;

		if (STATE_LOADER_USE_BYTE_BUFFER) {
			u = (unsigned) loadInteger();
		} else {
	        _bin_in->read((char *)(&u), sizeof(unsigned));
		}
        return u;
    } else {
        readNextToken();
        wchar_t* stopstring; 
        unsigned long x = wcstoul( token_string, &stopstring, 10);
        return (unsigned)x;
    }
}

float StateLoader::loadReal() {
    if (_binary) {
        float f;

		if (STATE_LOADER_USE_BYTE_BUFFER) {
			f = _stateByteBuffer->getFloat();
		} else {
			_bin_in->read((char *)(&f), sizeof(float));
		}
        return f;
    } else{
        readNextToken();
	
        // because Intel's compiler doesn't recognize _wtof -JCS  (boost::lexical_cast<float> would also work - AJF)
        char narrow_tok[101];
        strncpy(narrow_tok, OutputUtil::convertToChar(token_string), 100);
        return static_cast<float>(atof(narrow_tok));
    }
}

void *StateLoader::loadPointer() {
    if (_binary) {
        int i = loadInteger();
        return reinterpret_cast<void *>(static_cast<size_t>(i));
    } else {
        readNextToken();
        if (token_string[0] != L'@') {
			std::ostringstream err;
			err << "[Line " << _lineno << "] Expected pointer ID but got: \"" 
				<< OutputUtil::convertToChar(token_string) << "\"";
            throw UnexpectedInputException("StateLoader::loadPointer()", err.str().c_str());
        }
        return reinterpret_cast<void *>(static_cast<size_t>(_wtoi(token_string+1)));
    }
}

int StateLoader::beginList(const wchar_t *name) {
	int id = -1;
    if (_binary) {
        if (name != 0) {
			// Do for binary only because FullQuery loads from text!
			size_t replacement = (size_t) name;
			if (replacement >= IntegerCompressionStart && replacement < (IntegerCompressionStart + IntegerCompressionTokenCount)) {
				//std::cerr << "beginList " << (replacement - IntegerCompressionStart) << std::endl;
				int match = loadInteger(); 
				if (replacement != (size_t)match) {
					std::ostringstream err;
					err << "[Line " << _lineno << "] Expected object " 
						<< replacement << " but got " << match;
					throw UnexpectedInputException("BinaryStateLoader::beginList()", err.str().c_str());
				}
			} else {
				// Tricky syntax for quick skipping of a section.
				// Must back up 6 from beginning of matching name.
				wchar_t nameCopy[2048];
				wcscpy(nameCopy, name);
				const wchar_t* skipString = L" to ";
				wchar_t* toPtr = (wchar_t*)wcsstr(nameCopy, skipString);
				if (toPtr != NULL) {
					std::ostringstream ostr;
					ostr << "Substrings of '" << OutputUtil::convertToChar(nameCopy) << "[" << (void*)nameCopy << "]";
					ostr << "' divided by '" << OutputUtil::convertToChar(skipString) << "[" << (void*)toPtr << "]";
					*toPtr = L'\0';
					ostr << "' are '" << OutputUtil::convertToChar(nameCopy) << "' and '";
					ostr << OutputUtil::convertToChar(toPtr + wcslen(skipString)) << "'";
					ostr << std::endl;
					SessionLogger::info("SERIF") << ostr.str();
				}
    
				// check that the name matches and get the object id
				wchar_t* string = loadString();
				if (wcscmp(nameCopy, string) != 0) {
					std::ostringstream err;
					err << "[Line " << _lineno << "] Expected object ";
					err << OutputUtil::convertToChar(nameCopy) << " but got: ";
					err << OutputUtil::convertToChar(string);
					throw UnexpectedInputException("BinaryStateLoader::beginList()", err.str().c_str());
				}

				// Read byte pairs, looking for termination string.
				if (STATE_LOADER_USE_BYTE_BUFFER) {
					if (toPtr != NULL) {
						const wchar_t* findString = (wchar_t*)(toPtr + wcslen(skipString));
						std::ostringstream ostr;
						ostr << "Found " << OutputUtil::convertToChar(nameCopy) << " at "
							<< _stateByteBuffer << " finding " << OutputUtil::convertToChar(findString) << std::endl;
						SessionLogger::info("SERIF") << ostr.str();
						size_t findLength = wcslen(findString);
						size_t offset = 0;
						while (true) {
							wchar_t wc = _stateByteBuffer->getWideChar();
							if (wc == *(findString + offset)){
								if (offset > 0) {
									//std::cerr << "Found offset " << offset << " of " << OutputUtil::convertToChar(findString) << " at " << _stateByteBuffer << std::endl;
								}
								++offset;
								if (offset == findLength) {
									// Go back by subtheory number and string length. PLUS ID!
									//std::cerr << "Found " << OutputUtil::convertToChar(findString) << " at " << _stateByteBuffer << std::endl;
									*_stateByteBuffer -= (findLength * 2) + 4 + 2 + 4;
									int subtheory = _stateByteBuffer->peekInt();
									//std::cerr << "resetting to " << _stateByteBuffer << " value " << subtheory << std::endl;
									break;
								}
							} else {
								offset = 0;
							}
						}
					}
				}
			}
            id = loadInteger(); 
        }
    } else {
        ensureToken(L"(");
        if (name != 0) {
            // check that name matches and check for an object ID
            readNextToken();
            wchar_t *id_str = wcschr(token_string, L'@');
            if (id_str != 0) {
                // use id_str to get ID
                id = _wtoi(id_str+1);
                *id_str = L'\0'; // put null-terminator on name
            }
            if (wcscmp(name, token_string) != 0) {
				std::ostringstream err;
				err << "[Line " << _lineno << "] Expected object ";
				err << OutputUtil::convertToChar(name) << " but got: ";
				err << OutputUtil::convertToChar(token_string);
                throw UnexpectedInputException("StateLoader::beginList()", err.str().c_str());
            }
        }
	}
	return id;
}

void StateLoader::endList() {
	if (!_binary) {
        ensureToken(L")");
    }
}


void StateLoader::readNextToken() {
	wchar_t c;

	c = skipWhiteSpace();
	switch (c) {

		case L'#':
		{
			do {
				c = _text_in->get();
			} while (c != L'\n');
			++_lineno;
			readNextToken();
		}

		case L'"':
		{
			int i = 0;
			for (;;) {
				if (i == MAX_SERIF_TOKEN_LENGTH - 1)
					break;

				c = _text_in->get();
				if (c == L'"')
					break;
				if (c == L'\xfffe')
					c = L'"';

				token_string[i++] = c;
			}
			token_string[i] = L'\0';

			break;
		}

		case L'(':
		case L')':
		{
			token_string[0] = c;
			token_string[1] = L'\0';
			break;
		}

		default:
		{
			token_string[0] = c;
			int i = 1;
			for (;;) {
				if (i == MAX_SERIF_TOKEN_LENGTH - 1)
					break;

				c = _text_in->get();
				if (c == L' ' || c == L'\t' || c == L'\n' || c == L'\r' ||
					c == L'(' || c == L')' || c == L'#')
				{
					_text_in->putback(c);
					break;
				}

				token_string[i++] = c;
			}
			token_string[i] = L'\0';
		}
	}
}

wchar_t StateLoader::skipWhiteSpace() {
	wchar_t c;
	do {
		c = _text_in->get();
		if (c == '\n') { ++_lineno; }
	} while (c == L' ' || c == L'\t' || c == L'\n' || c == L'\r');
	return c;
}


void StateLoader::ensureToken(const wchar_t *str, const char *message) {
	if( _binary )
		loadString();
	else
		readNextToken();

	if (wcscmp(token_string, str) != 0) {
		char new_message[300];
		if (message != 0) {
			char narrow_str[101];
			strncpy(narrow_str, OutputUtil::convertToChar(str), 100);
			char narrow_tok[101];
			strncpy(narrow_tok, OutputUtil::convertToChar(token_string), 100);
			sprintf(new_message, message, narrow_str, narrow_tok);
		} else {
			strcpy(new_message, "Expected token `");
			strncat(new_message, OutputUtil::convertToChar(str), 100);
			strcat(new_message, "' but got: `");
			strncat(new_message, OutputUtil::convertToChar(token_string), 100);
			strcat(new_message, "'");
		}
		std::ostringstream err;
		err << "[Line " << _lineno << "] " << new_message;
		if (_binary)
			throw UnexpectedInputException("BinaryStateLoader::ensureToken()",
				err.str().c_str());
		else
			throw UnexpectedInputException("StateLoader::ensureToken()",
				err.str().c_str());
	}
}

void StateLoader::registerParse(const Parse* parse) {
	_parses.push_back(parse);
}

void StateLoader::clearParseRegistry() {
	_parses.clear();
}

const Parse* StateLoader::getParseByRoot(const SynNode* root) const {
	for (size_t i=0; i<_parses.size(); ++i) {
		if (_parses[i]->getRoot() == root)
			return _parses[i];
	}
	throw InternalInconsistencyException("StateLoader::getParseByRoot",
		"No parse found with that root node");
}

std::pair<int,int> StateLoader::getVersion() {
	if (_version == std::make_pair(0,0))
		throw InternalInconsistencyException("StateLoader::getVersion",
			"StateLoader is not currently reading a state tree (i.e., beginStateTree() has not been called)");
	return _version;
}
