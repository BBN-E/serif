// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef CONSOLE_SESSION_LOGGER_H
#define CONSOLE_SESSION_LOGGER_H

#include "Generic/common/SessionLogger.h"

#include <wchar.h>
#include <string>

class ConsoleSessionLogger : public SessionLogger {
public:
	// default; use this only when we're not sure whether ParamReader has been initialized yet
	ConsoleSessionLogger();
	ConsoleSessionLogger(const std::vector<std::wstring> & context_level_names, const wchar_t * output_prefix=L"[SERIF]");
	// for backwards compatibility
	ConsoleSessionLogger(int n_context_levels, const wchar_t **context_level_names, const wchar_t * output_prefix=L"[SERIF]");
	virtual ~ConsoleSessionLogger();
	virtual void displayString(const std::string &msg, const char *stream_id) { SessionLogger::displayString(msg, stream_id); }
	virtual void displayString(const std::wstring &msg, const char * stream_id=NULL);
	virtual void flush();
	virtual std::string getType(); // Useful for debugging
	static void disableDestructorNewline();
};


#endif
