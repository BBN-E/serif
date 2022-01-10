

// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TEE_SESSION_LOGGER_H
#define TEE_SESSION_LOGGER_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/FileSessionLogger.h"

#include <wchar.h>
#include <string>

/**
 * Mimics the behavior of the (UNIX) tee utility.
 * Writes to both a ConsoleSessionLogger and (if a nonempty filename is provided) a FileSessionLogger.
 *
 * @param append If true, log messages will be appended to existing log (if any); otherwise, any existing
 * log will be truncated when the TeeSessionLogger object is instantiated.
 *
 * @author afrankel@bbn.com
 * @date 2011.06.23
 **/
class TeeSessionLogger : public SessionLogger {
public:

	TeeSessionLogger(const wchar_t *file_name, const std::vector<std::wstring> & context_level_names, bool append);
	TeeSessionLogger(const char *file_name, const std::vector<std::wstring> & context_level_names, bool append);

	// for backwards compatibility
	TeeSessionLogger(const wchar_t *file_name, int n_context_levels,
				  const wchar_t **context_level_names, bool append);
	// for backwards compatibility
	TeeSessionLogger(const char *file_name, int n_context_levels,
				  const wchar_t **context_level_names, bool append);
	virtual ~TeeSessionLogger();

	/** Print message to console indicating how things went */
	virtual void displaySummary();

//	UTF8OutputStream &getStream() { return _log; }
	virtual void displayString(const std::string &msg, const char *stream_id) { SessionLogger::displayString(msg, stream_id); }
	virtual void displayString(const std::wstring &msg, const char * stream_id=NULL);
	virtual void flush();
	virtual std::string getType(); // Useful for debugging

protected:
	std::wstring _file_name;
	ConsoleSessionLogger * _console_logger;
	FileSessionLogger * _file_logger;
};


#endif

