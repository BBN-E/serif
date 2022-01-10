// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h" // This must be the first #include

#include "Generic/common/SessionLogger.h"
#include "Generic/common/NullSessionLogger.h"
#include "Generic/common/ConsoleSessionLogger.h"
#include "Generic/common/OutputUtil.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/UnicodeUtil.h"
#include "Generic/common/ParamReader.h"
#include "Generic/common/Symbol.h"

#include <string.h>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>
#include <boost/foreach.hpp>

using namespace std;

// Destructors for SessionLogger{Unsetter,Deleter,Restorer}.
// See header file and http://wiki.d4m.bbn.com/wiki/Serif/SessionLogger.
SessionLoggerUnsetter::~SessionLoggerUnsetter() {
	SessionLogger::unsetGlobalLogger();
}

SessionLoggerDeleter::~SessionLoggerDeleter() {
	SessionLogger::deleteGlobalLogger();
}

SessionLoggerRestorer::~SessionLoggerRestorer() {
   SessionLogger::deleteGlobalLogger();
   SessionLogger::restorePrevGlobalLogger();
}

// STATIC MEMBERS
// pointer to a logger that writes to console, file, both, or neither
SessionLogger * SessionLogger::_globalLoggerPtr = 0;
// pointer to a previously stored logger
SessionLogger * SessionLogger::_prevGlobalLoggerPtr = 0;


// default logger that takes anything written to it and swallows it without a trace (so to speak)
//NullSessionLogger SessionLogger::_defaultLogger;
// default logger that causes everything to be written to the console
ConsoleSessionLogger SessionLogger::_defaultLogger;

// This is for backwards compatibility only. Do not use it in new code.
SessionLogger * SessionLogger::logger = &SessionLogger::_defaultLogger;

// STATIC METHODS - SETTING UP/DELETING LOGGER
void SessionLogger::setGlobalLogger(SessionLogger * logger_in, bool store_old_logger/*=false*/) {
	if (store_old_logger) {
		_prevGlobalLoggerPtr = _globalLoggerPtr; // could be null
	}
	_globalLoggerPtr = logger_in;
	logger = logger_in; // for backwards compatibility only
}

bool SessionLogger::globalLoggerExists() {
	return _globalLoggerPtr != 0;}

void SessionLogger::deleteGlobalLogger() {
	delete _globalLoggerPtr; 
	_globalLoggerPtr = 0;
}

/**
 * This method does NOT delete any logger objects. To do that, the caller should call deleteGlobalLogger()
 * before calling this method.
 * @return True if a nonzero global logger was stored previously, false otherwise.
 **/
bool SessionLogger::restorePrevGlobalLogger() {
	_globalLoggerPtr = _prevGlobalLoggerPtr;
	logger = _globalLoggerPtr; // for backwards compatibility only
	return (_globalLoggerPtr != 0);
}			

// Used by a client to unregister the logger; client must then take care of deleting logger object separately.
void SessionLogger::unsetGlobalLogger() {
	_globalLoggerPtr = 0; }

// END OF METHODS FOR SETTING UP/DELETING LOGGER

// private method called internally by methods that write the individual messages
SessionLogger * SessionLogger::getLogger() {
	if (_globalLoggerPtr) {
		return _globalLoggerPtr;
	} else {
		return &_defaultLogger;
	}
}

/** This private structure is used to hold all of SessionLogger's internal variables.
  * This allows us to modify the implementation of the logger without requiring that
  * everything that imports SessionLogger.h be recompiled. */
struct SessionLogger::Internals {
	// The ordered list of context levels.
	std::vector<std::wstring> context_level_names;

	// Our current context.
	std::vector<std::wstring> context_info;

	// We will display any message at or above this threshold.
	LogLevel logLevelThreshold;

	// Determines whether IDs (in parentheses) are displayed before messages.
	bool show_ids;

	// Don't re-display the same context if we have multiple messages
	// in the same context;
	bool context_displayed;

	// If false, then log messages that contain newlines will have
	// those newlines replaced by "\\n" before they are printed.
	bool multiline_messages;

	// A string prepended to each line of the logger's output.
	std::wstring prefix; 

	// The number of messages of each type we have displayed.
	std::map<LogLevel, int> n_messages;

	// A list of message identifiers for messages that we should not report.
	std::vector<std::string> suppressedMessages;
	// A list of message identifiers for messages that we always report, even if its log level
	// falls below the log threshold.
	std::vector<std::string> forcedMessages;

	// A cache used to quickly check if a message should be suppressed.  Note
	// that the cache keys are compared based on identity; this is because we
	// assume that message identifiers will be inline string constants.
	struct MessageIdHash {size_t operator()(const char* s) const {return boost::hash_value((void*)s);}};
	struct MessageIdEq {bool operator()(const char* s1, const char* s2) const {return s1 == s2;}};
	typedef serif::hash_map<const char*, bool, MessageIdHash, MessageIdEq> MessageIdBoolMap;
	MessageIdBoolMap suppressedMessageCache;
	MessageIdBoolMap forcedMessageCache;

	// This form is easier to use (and slightly more efficient) than the one that follows.
	Internals(const std::vector<std::wstring> & context_level_name_vec, const wchar_t* output_prefix):
		context_level_names(context_level_name_vec)
	{
		init(output_prefix);
	}

	// This form is provided for the older (backwards compatible) SessionLogger constructor.
	Internals(size_t n_context_levels, const wchar_t **context_level_name_array, const wchar_t* output_prefix):
		context_level_names(context_level_name_array, context_level_name_array + n_context_levels)
	{	
		if (context_level_name_array == 0)
			throw InternalInconsistencyException("SessionLogger::Internals",
							 "context_level_name_array may not be NULL!");
		init(output_prefix);
	}

	void init(const wchar_t* output_prefix) {
		show_ids = false;
		context_displayed = false;
		multiline_messages = true;
		prefix = (output_prefix == NULL ? L"" : output_prefix);
		logLevelThreshold = INFO_LEVEL;
		context_info.resize(context_level_names.size()); // fills vector with blank strings
	}

	void init_from_params() {
		// Setting "log_show_ids: true" makes the display more verbose but allows you to immediately see the ID associated
		// with each message (useful for either debugging or determining which IDs to suppress).
		show_ids = ParamReader::getOptionalTrueFalseParamWithDefaultVal("log_show_ids", false);
		// For example, "log_ignore: rm_set_file_0,rm_set_dir_0" will cause messages with IDs "rm_set_file_0" 
		// and "rm_set_dir_0" to be suppressed.
		// There is no mechanism that forces these IDs to be unique, but the consequence of using duplicate IDs
		// is that you may unintentionally suppress more messages than you want, or when searching the code, you
		// may come across the wrong instance.
		suppressedMessages = ParamReader::getStringVectorParam("log_ignore");
		// For example, "log_force: dbg_xyz_0" will cause messages with ID "dbg_xyz_0" to be logged, even if it is 
		// a debug message and the log threshold is set to ignore debug messages in general.
		forcedMessages = ParamReader::getStringVectorParam("log_force");
		std::string logThreshold = ParamReader::getParam("log_threshold");
		if (!logThreshold.empty()) {
			if (boost::iequals(logThreshold, "DEBUG"))
				logLevelThreshold = DEBUG_LEVEL;
			else if (boost::iequals(logThreshold, "INFO"))
				logLevelThreshold = INFO_LEVEL;
			else if (boost::iequals(logThreshold, "USER_WARNING"))
				logLevelThreshold = USER_WARNING_LEVEL;
			else if (boost::iequals(logThreshold, "WARNING"))
				logLevelThreshold = WARNING_LEVEL;
			else if (boost::iequals(logThreshold, "UNEXPECTED_INPUT_ERROR_LEVEL"))
				logLevelThreshold = UNEXPECTED_INPUT_ERROR_LEVEL;
			else if (boost::iequals(logThreshold, "INTERNAL_INCONSISTENCY_ERROR_LEVEL"))
				logLevelThreshold = INTERNAL_INCONSISTENCY_ERROR_LEVEL;
			else if (boost::iequals(logThreshold, "ERROR"))
				logLevelThreshold = ERROR_LEVEL;
			else
				throw UnexpectedInputException("SessionLogger::SessionLogger",
					"Parameter 'log_threshold' must be set to 'DEBUG', 'INFO', 'WARNING' or ERROR");
		}
		multiline_messages = ParamReader::getOptionalTrueFalseParamWithDefaultVal("multiline_log_messages", true);
	}	
};

void SessionLogger::printGlobalLoggerInfo() {
	if (_globalLoggerPtr) {
		std::cerr << "Global logger is a " << _globalLoggerPtr->getType() << ", with log level " << _globalLoggerPtr->_impl->logLevelThreshold << "\n";
	} else {
		std::cerr << "Global logger pointer is null\n";;
	}
}

// This default constructor does not call ParamReader, so it is particularly useful when initializing
// a SessionLogger object before ParamReader has been initialized.
SessionLogger::SessionLogger():
	_impl(new SessionLogger::Internals(std::vector<std::wstring>(), L"")) {
}

// This form of the constructor is easier to use (and slightly more efficient).
SessionLogger::SessionLogger(const std::vector<std::wstring> & context_level_name_vec, const wchar_t* output_prefix):
_impl(new SessionLogger::Internals(context_level_name_vec, output_prefix)) {
	_impl->init_from_params();
}

// This form of the constructor is provided for backwards compatibility.
SessionLogger::SessionLogger(size_t n_context_levels, const wchar_t **context_level_names, const wchar_t* output_prefix):
_impl(new SessionLogger::Internals(n_context_levels, context_level_names, output_prefix)) {
	_impl->init_from_params();
}

SessionLogger::~SessionLogger() {
	delete _impl;
}

void SessionLogger::updateContext(size_t context_level, const wchar_t *context_info) {
	getLogger()->updateLocalContext(context_level, context_info);
}

// This is the version that does the work for updateContext() and the other version of updateLocalContext().
void SessionLogger::updateLocalContext(size_t context_level, const wchar_t *context_info) {
	size_t levels_defined = (_impl->context_level_names).size();
	if (levels_defined == 0) {
		return; // for NullSessionLogger
	} else if (context_level >= levels_defined) {
		warn("bad_context_level_0") << "Ignoring attempt to update context level " << context_level
			<< "; only " << levels_defined << " levels are defined.";
	} else {
		_impl->context_info[context_level] = context_info;

		// erase information about all lower context levels
		for (size_t i = context_level + 1; i < _impl->context_info.size(); i++)
			_impl->context_info[i].clear();
		_impl->context_displayed = false;
	}
}

void SessionLogger::updateContext(size_t context_level, const char *context_info) {
	getLogger()->updateLocalContext(context_level, context_info);
}

void SessionLogger::updateLocalContext(size_t context_level, const char *context_info) {
    updateLocalContext(context_level, UnicodeUtil::toUTF16StdString(context_info, UnicodeUtil::REPLACE_ON_ERROR).c_str());
}


void SessionLogger::beginMessage() {
	_impl->n_messages[INFO_LEVEL]++;
	*this << L"\n";
}

void SessionLogger::beginEvent() {
	_impl->n_messages[DEBUG_LEVEL]++;
	*this << L"\nEvent:\n";
	writeContext();
	*this << L"(--) ";
}

void SessionLogger::beginWarning() {
	_impl->n_messages[WARNING_LEVEL]++;
	*this << L"\nWarning:\n";
	writeContext();
	*this << L"(**) ";
}

void SessionLogger::beginError() {
	_impl->n_messages[ERROR_LEVEL]++;
	*this << L"\nError:\n";
	writeContext();
	*this << L"(!!) ";
}

void SessionLogger::displaySummary() {
	int _n_warnings = _impl->n_messages[WARNING_LEVEL] + _impl->n_messages[USER_WARNING_LEVEL];
	int _n_errors = _impl->n_messages[ERROR_LEVEL] + _impl->n_messages[UNEXPECTED_INPUT_ERROR_LEVEL] + _impl->n_messages[INTERNAL_INCONSISTENCY_ERROR_LEVEL];
	if (_n_warnings == 0 && _n_errors == 0) {
		cout << "Session completed with no warnings.\n";
	}
	else {
		cout << "Session completed with " << _n_errors << " error(s) and "
			 << _n_warnings << " warning(s).\n";
		cout << "Check session log for warning messages.\n";
	}
}

void SessionLogger::writeContext() {
	for (size_t i = 0;
		 i < _impl->context_info.size() && _impl->context_info[i][0] != L'\0';
		 i++)
	{
		for (size_t j = 0; j <= i; j++)
			*this << L"  ";
		*this << L"- " << _impl->context_level_names[i] << L": "
					   << _impl->context_info[i] << "\n";
	}
}

SessionLogger &SessionLogger::operator<<(const char *s) {
	displayString(UnicodeUtil::toUTF16StdString(s, UnicodeUtil::REPLACE_ON_ERROR), /*stream_id=*/NULL);
	if (strchr(s, L'\n')) flush();
	return *this;
}

SessionLogger &SessionLogger::operator<<(const wchar_t *s) {
	displayString(s, /*stream_id=*/NULL);
	if (wcschr(s, L'\n')) flush();
	return *this;
}

SessionLogger &SessionLogger::operator<<(const string &s) {
	displayString(UnicodeUtil::toUTF16StdString(s, UnicodeUtil::REPLACE_ON_ERROR), /*stream_id=*/NULL);
	if (strchr(s.c_str(), L'\n')) flush();
	return *this;
}

SessionLogger &SessionLogger::operator<<(const wstring &s) {
	displayString(s, /*stream_id=*/NULL);
	if (wcschr(s.c_str(), L'\n')) flush();
	return *this;
}

SessionLogger &SessionLogger::operator<<(bool b) {
	displayString(boost::lexical_cast<std::wstring>(b), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(char c) {
	displayString(boost::lexical_cast<std::wstring>(c), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(short i) {
	displayString(boost::lexical_cast<std::wstring>(i), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(int i) {
	displayString(boost::lexical_cast<std::wstring>(i), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(long i) {
	displayString(boost::lexical_cast<std::wstring>(i), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(unsigned short i) {
	displayString(boost::lexical_cast<std::wstring>(i), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(unsigned int i) {
	displayString(boost::lexical_cast<std::wstring>(i), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(unsigned long i) {
	displayString(boost::lexical_cast<std::wstring>(i), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(unsigned long long i) {
	displayString(boost::lexical_cast<std::wstring>(i), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(double r) {
	displayString(boost::lexical_cast<std::wstring>(r), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(const UnrecoverableException& exc) {
	this->reportError() << exc.getSource() << ": " << exc.getMessage();
	return *this;
}

// Use this to accept, but ignore, std::endl. Note that it will cause other 
// stream manipulators (e.g., left, right, hex) to be ignored as well, but
// they're never used in the codebase (except on ostringstream objects).
SessionLogger &SessionLogger::operator<<(std::ostream&(*f)(std::ostream&)) {
	return *this;
}

SessionLogger &SessionLogger::operator<<(const std::ostringstream & ostr) {
	displayString(ostr.str(), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(const std::wostringstream & ostr) {
	displayString(ostr.str(), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(const Symbol &s) {
	displayString(s.to_string(), /*stream_id=*/NULL);
	return *this;
}

SessionLogger &SessionLogger::operator<<(const boost::gregorian::date &d) {
	displayString(boost::gregorian::to_iso_extended_string(d), /*stream_id=*/NULL);
	return *this;
}

namespace {
	const wchar_t *LOG_LEVEL_PREFIXES[SessionLogger::NUM_LEVELS] = {
		L"Debug: ",           // DEBUG_LEVEL, 
		L"",                  // INFO_LEVEL, 
		L"(!!) Warning: ",    // WARNING_LEVEL,		
		L"(!!) Warning: ",    // USER_WARNING_LEVEL, 
		L"(**) Error: ",      // UNEXPECTED_INPUT_ERROR_LEVEL,
		L"(**) Error: ",      // INTERNAL_INCONSISTENCY_ERROR_LEVEL,
		L"(**) Error: ",      // ERROR_LEVEL
	};

	void indent(std::wostringstream& out, size_t levels) {
		for (size_t j=0; j<levels; ++j) out << L" ";
	}
}

// Static methods.
bool SessionLogger::dbg_or_msg_enabled(const char* identifier) {
	return enabledAtLevel(identifier, DEBUG_LEVEL);}
bool SessionLogger::info_or_msg_enabled(const char* identifier) {
	return enabledAtLevel(identifier, INFO_LEVEL);}
bool SessionLogger::warn_or_msg_enabled(const char* identifier) {
	return enabledAtLevel(identifier, WARNING_LEVEL);}
// We expect this to be used infrequently, if at all, as the user will probably always want to see error messages.
bool SessionLogger::err_or_msg_enabled(const char* identifier) {
	return enabledAtLevel(identifier, ERROR_LEVEL);}
// We expect these to be used very infrequently, if at all.
bool SessionLogger::user_warning_or_msg_enabled(const char* identifier) {
	return enabledAtLevel(identifier, USER_WARNING_LEVEL);}
bool SessionLogger::err_unexpected_input_or_msg_enabled(const char* identifier) {
	return enabledAtLevel(identifier, UNEXPECTED_INPUT_ERROR_LEVEL);}
bool SessionLogger::err_internal_inconsistency_or_msg_enabled(const char* identifier) {
	return enabledAtLevel(identifier, INTERNAL_INCONSISTENCY_ERROR_LEVEL);}

// Static. See enabledAtLocalLevel() (which is nonstatic) for details.
bool SessionLogger::enabledAtLevel(const char* identifier, SessionLogger::LogLevel level) {
	return getLogger()->enabledAtLocalLevel(identifier, level);
}

// Returns false if the message with the specified identifier is suppressed, or if the specified level
// is below the log threshold (and the message is not in the list of messages that are forced to be displayed,
// specified via a param read by ParamReader). Returns true otherwise.
// It's a bad idea to include a message in both the suppressed (log_ignore) and forced (log_force) lists,
// but if you do, it will be suppressed.
bool SessionLogger::enabledAtLocalLevel(const char* identifier, SessionLogger::LogLevel level) const {
	if (isMessageSuppressed(identifier)) {
		return false;
	} else if ((level < _impl->logLevelThreshold) && !isMessageForced(identifier)) {
		return false;
	} else {
		return true;
	}
}

void SessionLogger::reportMessage(SessionLogger::LogLevel level, const char* identifier, const char * stream_id,
								  const std::wstring &msg, bool add_newline) 
{
	if (!enabledAtLevel(identifier, level) || msg.empty()) {
		return;
	}
	// increment count of messages at level
	_impl->n_messages[level]++;
	wostringstream out;
	// Display the current context, starting at the last level we displayed.
	if (_impl->multiline_messages) {
		if (!_impl->context_displayed) {
			for (size_t i=0; i<_impl->context_info.size() && !_impl->context_info[i].empty(); ++i) {
				out << _impl->prefix;
				indent(out, i*2);
				out << _impl->context_level_names[i] << L": " 
					<< _impl->context_info[i] << L"\n"; 
			}
			_impl->context_displayed = true;
		}
	} else {
		out << LOG_LEVEL_PREFIXES[level];
		out << _impl->prefix << L"[";
		for (size_t i=0; i<_impl->context_info.size() && !_impl->context_info[i].empty(); ++i) {
			if (i>0) out << L"; ";
			out << _impl->context_level_names[i] << L"=" << _impl->context_info[i];
		}
		out << L"] ";
	}
	// Display the message.  Indent individual lines.
	std::vector<std::wstring> lines;
	boost::algorithm::split(lines, msg, boost::is_any_of(L"\n"), boost::token_compress_on);
	size_t first_nonempty_index=0;
	// Find the index of the first nonempty line. If all lines are empty, no problem: we'll increment
	// first_nonempty_index past lines.size() and the loop that prints output will be skipped.
	while (first_nonempty_index<lines.size() && lines[first_nonempty_index].empty()) {
		++first_nonempty_index;
	}
	for (size_t i=first_nonempty_index; i<lines.size(); ++i) {
		if (lines[i].empty())
			continue;
		if (i>first_nonempty_index) {
			if (add_newline) {
				out << (_impl->multiline_messages?L"\n":L"\\n");
			} else {
				out << L" ";
			}
		}
		out << _impl->prefix;
		if (_impl->multiline_messages) {
			for (size_t j=0; j<_impl->context_info.size() && !_impl->context_info[j].empty(); ++j)
				out << L"  ";
			if (i == first_nonempty_index) 
				out << LOG_LEVEL_PREFIXES[level];
			else
				indent(out, wcslen(LOG_LEVEL_PREFIXES[level]));
			if (_impl->show_ids && identifier != NULL)
				out << L"(" << UnicodeUtil::toUTF16StdString(identifier, UnicodeUtil::REPLACE_ON_ERROR) << L") ";
		}
		out << lines[i];
	}
	if (add_newline) {
		out << L'\n';
	}
	displayString(out.str(), stream_id);
	flush();
}

bool SessionLogger::isMessageSuppressed(const char* identifier) const {
	if (identifier == NULL) return false;
	if (_impl->suppressedMessageCache.find(identifier) != _impl->suppressedMessageCache.end())
		return _impl->suppressedMessageCache[identifier];

	bool suppressed = false;
	for (size_t i = 0; i < _impl->suppressedMessages.size(); i++) {
		std::string msg_id = _impl->suppressedMessages[i];

		if (boost::iequals(msg_id, identifier)) {
			suppressed = true;
			break;
		}
	}
	if (_impl->suppressedMessageCache.size() < 1000)
		_impl->suppressedMessageCache[identifier] = suppressed;
	return suppressed;
}

bool SessionLogger::isMessageForced(const char* identifier) const {
	if (identifier == NULL) return false;
	if (_impl->forcedMessageCache.find(identifier) != _impl->forcedMessageCache.end())
		return _impl->forcedMessageCache[identifier];

	bool forced = false;
	for (size_t i = 0; i < _impl->forcedMessages.size(); i++) {
		std::string msg_id = _impl->forcedMessages[i];
		if (boost::iequals(msg_id, identifier)) {
			forced = true;
			break;
		}
	}
	if (_impl->forcedMessageCache.size() < 1000)
		_impl->forcedMessageCache[identifier] = forced;
	return forced;
}


SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(const char *s) {
	if (_msg) *_msg << UnicodeUtil::toUTF16StdString(s, UnicodeUtil::REPLACE_ON_ERROR); return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(const wchar_t *s) { 
	if (_msg) *_msg << s; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(const std::string &s) { 
	if (_msg) *_msg << UnicodeUtil::toUTF16StdString(s, UnicodeUtil::REPLACE_ON_ERROR); return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(const std::wstring &s) {
	if (_msg) *_msg << s; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(bool b) { 
	if (_msg) *_msg << b; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(char c) { 
	if (_msg) *_msg << c; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(short i) { 
	if (_msg) *_msg << i; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(int i) { 
	if (_msg) *_msg << i; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(long i) { 
	if (_msg) *_msg << i; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(unsigned short i) { 
	if (_msg) *_msg << i; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(unsigned int i) { 
	if (_msg) *_msg << i; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(unsigned long i) { 
	if (_msg) *_msg << i; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(unsigned long long i) { 
	if (_msg) *_msg << i; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(double r) { 
	if (_msg) *_msg << r; return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(const UnrecoverableException &exc) {
	if (dynamic_cast<const InternalInconsistencyException*>(&exc))
		_level = INTERNAL_INCONSISTENCY_ERROR_LEVEL;
	else if (dynamic_cast<const UnexpectedInputException*>(&exc))
		_level = UNEXPECTED_INPUT_ERROR_LEVEL;
	if (_msg) *_msg << exc.getSource() << ": " << exc.getMessage(); return *this; 
}
// for ignoring std::endl
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(std::ostream&(*f)(std::ostream&)) { 
	return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(const std::ostringstream & ostr) { 
	return operator<<(ostr.str()); }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(const std::wostringstream & ostr) { 
	if (_msg) *_msg << ostr.str(); return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(const Symbol &s) {
	if (_msg) *_msg << s.to_debug_string(); return *this; }
SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::operator<<(const boost::gregorian::date &d) {
	if (_msg) *_msg << UnicodeUtil::toUTF16StdString(boost::gregorian::to_iso_extended_string(d), UnicodeUtil::REPLACE_ON_ERROR); return *this; }

SessionLogger::LogMessageMaker::LogMessageMaker(LogLevel level): 
	_level(level), _msg(), _identifier(),
	_stream_id(0), _add_trail_newline(true) 
{
	if (enabledAtLevel(NULL, level))
		_msg.reset(_new std::wostringstream());
}

SessionLogger::LogMessageMaker::LogMessageMaker(const LogMessageMaker &other):
	_level(other._level), _msg(other._msg), _identifier(other._identifier), _stream_id(other._stream_id), 
		_add_trail_newline(other._add_trail_newline) {}

SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::with_id(const char* log_message_id) {
	_identifier = log_message_id;
	if ((!_msg) && enabledAtLevel(log_message_id, _level))
		_msg.reset(_new std::wostringstream());
	return *this;
}

SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::with_stream_id(const char* stream_id) {
	_stream_id = stream_id;
	return *this;
}

SessionLogger::LogMessageMaker& SessionLogger::LogMessageMaker::add_trail_newline(bool add) {
	_add_trail_newline = add;
	return *this;
}

SessionLogger::LogMessageMaker::~LogMessageMaker() {
	// If we're the last one with a pointer to this message, then report it 
	// before we destroy ourselves.
	if (_msg.unique())
		getLogger()->reportMessage(_level, _identifier, _stream_id, _msg->str(), _add_trail_newline);
}

// CONVENIENCE METHODS
// Example of usage: 
//		SessionLogger::info("patt_file_0") << "Reading pattern file " << filename << "\n";
// Note that they expect an ID to be defined. We want to encourage the use of IDs 
// everywhere so that the corresponding messages can be suppressed, or the relevant code 
// can be found via a search (which is easier than looking for the text of the message).

// We expect these four (which have short names) to be called frequently.
SessionLogger::LogMessageMaker SessionLogger::dbg(const char* log_message_id, const char* stream_id/*=NULL*/) {
	return getLogger()->reportDebugMessage().with_id(log_message_id).with_stream_id(stream_id);}
SessionLogger::LogMessageMaker SessionLogger::info(const char* log_message_id, const char* stream_id/*=NULL*/) {
	return getLogger()->reportInfoMessage().with_id(log_message_id).with_stream_id(stream_id);}
SessionLogger::LogMessageMaker SessionLogger::warn(const char* log_message_id, const char* stream_id/*=NULL*/) {
	return getLogger()->reportWarning().with_id(log_message_id).with_stream_id(stream_id);}
SessionLogger::LogMessageMaker SessionLogger::err(const char* log_message_id, const char* stream_id/*=NULL*/) {
	return getLogger()->reportError().with_id(log_message_id).with_stream_id(stream_id);}

// We expect these four (which have longer names) to be called less frequently.
SessionLogger::LogMessageMaker SessionLogger::warn_user(const char* log_message_id, const char* stream_id/*=NULL*/) { 
	return getLogger()->reportUserWarning().with_id(log_message_id).with_stream_id(stream_id); }
SessionLogger::LogMessageMaker SessionLogger::err_unexpected_input(const char* log_message_id, const char* stream_id/*=NULL*/) { 
	return getLogger()->reportUnexpectedInputError().with_id(log_message_id).with_stream_id(stream_id); }
SessionLogger::LogMessageMaker SessionLogger::err_internal_inconsistency(const char* log_message_id, const char* stream_id/*=NULL*/) { 
	return getLogger()->reportInternalInconsistencyError().with_id(log_message_id).with_stream_id(stream_id); }

bool SessionLogger::is_wcerr_synonym(const char * stream_id) {
	if (stream_id == NULL) {
		return false;
	} else if (strcmp(stream_id, "cerr") == 0 ||
		strcmp(stream_id, "wcerr") == 0 ||
		strcmp(stream_id, "std::cerr") == 0 || 
		strcmp(stream_id, "std::wcerr") == 0) {
			return true;
	} else {
		return false;
	}
}

bool SessionLogger::is_wcout_synonym(const char * stream_id) {
	if (stream_id == NULL) {
		return false;
	} else if (strcmp(stream_id, "cout") == 0 ||
		strcmp(stream_id, "wcout") == 0 ||
		strcmp(stream_id, "std::cout") == 0 || 
		strcmp(stream_id, "std::wcout") == 0) {
			return true;
	} else {
		return false;
	}
}

void SessionLogger::displayString(const std::string &msg, const char *stream_id) {
	displayString(UnicodeUtil::toUTF16StdString(msg, UnicodeUtil::REPLACE_ON_ERROR), stream_id);
}

std::string SessionLogger::getType() {
	return "SessionLogger";
}
