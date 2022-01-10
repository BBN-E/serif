// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/StringTransliterator.h"

#include <iostream>
#include <vector>
#include <string>

namespace {
	bool disable_destructor_newline(false);
}

ConsoleSessionLogger::ConsoleSessionLogger()
: SessionLogger() {
}

ConsoleSessionLogger::ConsoleSessionLogger(const std::vector<std::wstring> & context_level_names, 
										   const wchar_t * output_prefix/*=L"[SERIF]"*/)
	: SessionLogger(context_level_names, output_prefix) {
}

// for backwards compatibility
ConsoleSessionLogger::ConsoleSessionLogger(int n_context_levels, const wchar_t **context_level_names, 
										   const wchar_t * output_prefix/*=L"[SERIF]"*/) 
	: SessionLogger(n_context_levels, context_level_names, output_prefix) {
}

void ConsoleSessionLogger::disableDestructorNewline() {
	disable_destructor_newline = true;
}


ConsoleSessionLogger::~ConsoleSessionLogger() {
	if (!disable_destructor_newline)
		std::cout << "\n";
}

void ConsoleSessionLogger::displayString(const std::wstring &msg, const char * stream_id/*=NULL*/) {
	size_t num_bytes = msg.size()*5 + 1; // Unicode is escaped as "\x1234"
	char *str = _new char[num_bytes];
	StringTransliterator::transliterateToEnglish(str, msg.c_str(), static_cast<int>(num_bytes));
	if (is_wcout_synonym(stream_id)) {
		std::wcout << str;
	} else {
		// We don't check is_wcerr_synonym here because we always default to wcerr if we don't find
		// a stream ID match.
		std::wcerr << str;
	}
	delete[] str;
}

void ConsoleSessionLogger::flush() {
	std::wcout.flush();
}

std::string ConsoleSessionLogger::getType() {
	return "ConsoleSessionLogger";
}
