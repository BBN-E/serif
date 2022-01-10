// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/DebugStream.h"
#include "Generic/common/ParamReader.h"
#include <iostream>


using namespace std;

//DebugStream DebugStream::nullStream;
DebugStream DebugStream::referenceResolverStream;

DebugStream::DebugStream() {
	_active = false;
	_echo_to_console = false;
	_indent = 0;
}

DebugStream::DebugStream(Symbol param_name, bool echo_to_console, bool append) {
	init(param_name, echo_to_console, append);
}

void DebugStream::init(Symbol param_name, bool echo_to_console, bool append) {
	_indent = 0;
	std::string buffer = ParamReader::getParam(param_name.to_debug_string());
	_active = (!buffer.empty());
	if (_active) {
		bool result = openFile(buffer.c_str(), echo_to_console, append);
		if (!result)
			_active = false;
	}
}

bool DebugStream::openFile(const char *file, bool echo_to_console, bool append) {
	_echo_to_console = echo_to_console;
	_out.open(file, append);
	return !_out.fail();
}
	
DebugStream::~DebugStream() {
	if (_active)
		_out.close();
}

DebugStream& DebugStream::operator<< (const std::wstring& str) {
	if(_active) _out << str;
//	if(_echo_to_console) cout << str;
	return *this;
}

DebugStream& DebugStream::operator<< (const wchar_t* str) {
	if(_active) _out << str;
//	if(_echo_to_console) cout << str;
	return *this;
}

DebugStream& DebugStream::operator<< (const char* str) {
	if (_active) { //|| _echo_to_console) {
		if(strcmp(str, "\n")==0) {
			for (int i=0; i<_indent; i++)  {
				if (_active) _out << "    ";
//				if (_echo_to_console) cout << "    ";
			}
		}
		if(_active) _out << str;
//		if(_echo_to_console) cout << str;
	}
	return *this;
}

DebugStream& DebugStream::operator<< (int i) {
	if(_active) _out << i;
//	if(_echo_to_console) cout << i;
	return *this;
}

DebugStream& DebugStream::operator<< (double i) {
	if(_active) _out << i;
//	if(_echo_to_console) cout << i;
	return *this;
}
