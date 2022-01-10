// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NULL_SESSION_LOGGER_H
#define NULL_SESSION_LOGGER_H

#include "Generic/common/SessionLogger.h"

/** A SessionLogger that discards all session log messages.  This
  * is used when we do not wish to create any files on disk or 
  * to send messages to the console -- e.g., when 
  * SessionProgram.hasExperimentDir()==false. */
class NullSessionLogger : public SessionLogger {
public:
	NullSessionLogger(): SessionLogger() {}
	void displaySummary() {}
protected:
	virtual void displayString(const std::string &msg, const char *stream_id) { SessionLogger::displayString(msg, stream_id); }
	virtual void displayString(const std::wstring &msg, const char * stream_id=NULL) {}
	virtual void flush() {}
	virtual std::string getType() { return "NullSessionLogger"; } // Useful for debugging
};

#endif

