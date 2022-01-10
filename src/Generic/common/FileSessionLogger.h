

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef FILE_SESSION_LOGGER_H
#define FILE_SESSION_LOGGER_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/SessionLogger.h"

#include <wchar.h>
#include <string>


/**
 * Writes to a log file.
 *
 * @param append If true, log messages will be appended to existing log (if any); otherwise, any existing
 * log will be truncated when the FileSessionLogger object is instantiated.
 *
 **/
class FileSessionLogger : public SessionLogger {
public:
	
	FileSessionLogger(const wchar_t *file_name, const std::vector<std::wstring> & context_level_names, bool append=true);
	FileSessionLogger(const char *file_name, const std::vector<std::wstring> & context_level_names, bool append=true);

	// These two are included for backwards compatibility.
	FileSessionLogger(const wchar_t *file_name, int n_context_levels,
				  const wchar_t **context_level_names, bool append=true);
	FileSessionLogger(const char *file_name, int n_context_levels,
				  const wchar_t **context_level_names, bool append=true);
	virtual ~FileSessionLogger();

	/** Print message to console indicating how things went */
	virtual void displaySummary();

	UTF8OutputStream &getStream() { return _log; }
	virtual void displayString(const std::string &msg, const char *stream_id) {
		SessionLogger::displayString(msg, stream_id);	
	}
	virtual void displayString(const std::wstring &msg, const char * stream_id=NULL);
	virtual void flush();
	virtual std::string getType(); // Useful for debugging

protected:

	std::wstring _file_name;
	UTF8OutputStream _log;
};


#endif

