// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/FileSessionLogger.h"
#include "Generic/common/OutputUtil.h"

#include <string.h>
#include <iostream>


using namespace std;

//file_name is const wchar_t *
FileSessionLogger::FileSessionLogger(const wchar_t *file_name, const std::vector<std::wstring> & context_level_names, 
									 bool append/*=true*/)
	: SessionLogger(context_level_names),
	  _file_name(file_name)
{
	_log.open(file_name, append);
}

//file_name is const char *
FileSessionLogger::FileSessionLogger(const char *file_name, const std::vector<std::wstring> & context_level_names, 
									 bool append/*=true*/)
	: SessionLogger(context_level_names),
	  _file_name(file_name, file_name+strlen(file_name))
{
	_log.open(file_name, append);
}

// for backwards compatibility (n_context_levels and context_level_names specified separately)
FileSessionLogger::FileSessionLogger(const wchar_t *file_name, int n_context_levels,
					        		 const wchar_t **context_level_names, bool append /*=true*/)
	: SessionLogger(n_context_levels, context_level_names),
	  _file_name(file_name)
{
	_log.open(file_name, append);
}

// for backwards compatibility (n_context_levels and context_level_names specified separately)
FileSessionLogger::FileSessionLogger(const char *file_name, int n_context_levels,
					        		 const wchar_t **context_level_names, bool append /*=true*/)
	: SessionLogger(n_context_levels, context_level_names),
	  _file_name(file_name, file_name+strlen(file_name))
{
	_log.open(file_name, append);
}

FileSessionLogger::~FileSessionLogger() {
	_log << L"\n";
	_log.close();
}

void FileSessionLogger::displaySummary() {
	SessionLogger::displaySummary();
	wcout << L"Session log is in: " << _file_name << L"\n";
}

// Ignore stream_id. Revisit if we later handle multiple stream IDs for FileSessionLogger.
void FileSessionLogger::displayString(const std::wstring &msg, const char *stream_id/*=NULL*/) {
	_log << msg;
}

void FileSessionLogger::flush() {
	_log.flush();
}

std::string FileSessionLogger::getType() {
	return "FileSessionLogger";
}
