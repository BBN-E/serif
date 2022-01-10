// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/TeeSessionLogger.h"
#include "Generic/common/OutputUtil.h"

#include <string.h>
#include <iostream>


using namespace std;

TeeSessionLogger::TeeSessionLogger(const wchar_t *file_name, const std::vector<std::wstring> & context_level_names, 
								   bool append)
	: SessionLogger(context_level_names),	
	_file_name(file_name),
	_console_logger(NULL),
	_file_logger(NULL)
{
	_console_logger = new ConsoleSessionLogger(context_level_names);
	if (!_file_name.empty()) {
		_file_logger = new FileSessionLogger(file_name, context_level_names, append);
	}
}

TeeSessionLogger::TeeSessionLogger(const char *file_name, const std::vector<std::wstring> & context_level_names, 
								   bool append)
	: SessionLogger(context_level_names),
	  _file_name(file_name, file_name+strlen(file_name)),
	  _console_logger(NULL),
	  _file_logger(NULL)
{
	_console_logger = new ConsoleSessionLogger(context_level_names);
	if (!_file_name.empty()) {
		_file_logger = new FileSessionLogger(file_name, context_level_names, append);
	}
}

// for backwards compatibility
TeeSessionLogger::TeeSessionLogger(const wchar_t *file_name, int n_context_levels,
					        		 const wchar_t **context_level_names, bool append)
	: SessionLogger(n_context_levels, context_level_names),	
	_file_name(file_name),
	_console_logger(NULL),
	_file_logger(NULL)
{
	_console_logger = new ConsoleSessionLogger(n_context_levels, context_level_names);
	if (!_file_name.empty()) {
		_file_logger = new FileSessionLogger(file_name, n_context_levels, context_level_names, append);
	}
}

// for backwards compatibility
TeeSessionLogger::TeeSessionLogger(const char *file_name, int n_context_levels,
					        		 const wchar_t **context_level_names, bool append)
	: SessionLogger(n_context_levels, context_level_names),
	  _file_name(file_name, file_name+strlen(file_name)),
	  _console_logger(NULL),
	  _file_logger(NULL)
{
	_console_logger = new ConsoleSessionLogger(n_context_levels, context_level_names);
	if (!_file_name.empty()) {
		_file_logger = new FileSessionLogger(file_name, n_context_levels, context_level_names, append);
	}
}

TeeSessionLogger::~TeeSessionLogger() {
	* _console_logger << L"\n";
	if (_file_logger) {
		* _file_logger << L"\n";
		delete _file_logger;
	}
	delete _console_logger;
}

void TeeSessionLogger::displaySummary() {
	_console_logger->displaySummary();
	if (_file_logger) {
		_file_logger->displaySummary();
		wcout << L"Session log is in: " << _file_name << L"\n";
	}
}

void TeeSessionLogger::displayString(const std::wstring &msg, const char *stream_id) {
	if (stream_id == NULL) { // most common case
		_console_logger->displayString(msg, NULL);
		if (_file_logger) {
			_file_logger->displayString(msg, NULL);
		}
	} else if (is_wcerr_synonym(stream_id) || is_wcout_synonym(stream_id)) {
		_console_logger->displayString(msg, stream_id);
	} else if (_file_logger) {
		_file_logger->displayString(msg, stream_id);
	} else { 
		// Caller didn't specify a valid "wcerr" or "wcout" synonym, but we take pity and rather
		// than let the message drop silently, we write it to wcout.
		_console_logger->displayString(msg, "wcout");
	}
}

void TeeSessionLogger::flush() {
	_console_logger->flush();
	if (_file_logger) {
		_file_logger->flush();
	}
}

std::string TeeSessionLogger::getType() {
	return "TeeSessionLogger";
}
