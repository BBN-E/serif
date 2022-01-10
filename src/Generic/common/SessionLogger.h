// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef SESSION_LOGGER_H
#define SESSION_LOGGER_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/hash_map.h"

#include <wchar.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <sstream>
#include <vector>
#include <map>

class UnrecoverableException;
class NullSessionLogger;
class ConsoleSessionLogger;
class Symbol;

/**
 * Utility class that can be used to ensure that unsetGlobalLogger() is called when
 * a SessionLogger object instantiated on the stack is destroyed,
 * provided that the client code instantiates a SessionLoggerUnsetter object that will go
 * out of scope when the SessionLogger is no longer valid.
 * @see http://wiki.d4m.bbn.com/wiki/Serif/SessionLogger
 * @author afrankel@bbn.com
 * @date 2011.07.13
 **/
class SessionLoggerUnsetter {
public:
   SessionLoggerUnsetter() {}
   ~SessionLoggerUnsetter(); // calls SessionLogger::unsetGlobalLogger()
};

/**
 * Utility class that can be used to ensure that deleteGlobalLogger() is called when
 * a SessionLogger object instantiated on the heap is destroyed,
 * provided that the client code instantiates a SessionLoggerDeleter object that will go
 * out of scope when the SessionLogger is no longer valid.
 * @see http://wiki.d4m.bbn.com/wiki/Serif/SessionLogger
 * @author afrankel@bbn.com
 * @date 2011.07.13
 **/
class SessionLoggerDeleter {
public:
   SessionLoggerDeleter() {}
   ~SessionLoggerDeleter(); // calls SessionLogger::deleteGlobalLogger()
};

/**
 * Utility class that can be used to ensure that deleteGlobalLogger(), followed by 
 * restorePrevGlobalLogger(), is called when a SessionLogger object instantiated 
 * on the heap (and passed to setGlobalLogger() with store_old_logger=true) is 
 * destroyed, provided that the client code instantiates a SessionLoggerDeleter 
 * object that will go out of scope when the SessionLogger is no longer valid.
 * @see http://wiki.d4m.bbn.com/wiki/Serif/SessionLogger
 * @author afrankel@bbn.com
 * @date 2011.07.13
 **/
class SessionLoggerRestorer {
public:
   SessionLoggerRestorer() {}
   ~SessionLoggerRestorer(); // calls deleteGlobalLogger(), then restorePrevGlobalLogger()
};

/** SessionLogger: Log session information in an informative format.
  * Use SessionLogger::logger to access the currently active logger
  * object.  To generate a log message, call one of the reporting 
  * methods (such as SessionLogger::warn()), and then use
  * operator<< to specify the contents of the message.  For example:
  *
  *   SessionLogger::warn("num_foos_0") 
  *       << "num_foos is too large (" << num_foos << ")";
  *
  * A trailing newline will automatically be appended to the message. 
  * The identifier "num_foos_0" will be associated with this log
  * message (which can be used to suppress it, via the "log_ignore"
  * parameter).
  *
  * The logger currently defines the following message levels:
  *
  * MESSAGES: Contain general information.
  *   - DebugMessage: A message that is used for debugging.  By default,
  *     these messages are not displayed. Used via dbg().
  *   - InfoMessage: A general information message.  For example, this
  *     is used to report the conents of the parameter file. Used via info().
  *
  * WARNINGS: Indicate a non-fatal problem.
  *   - Warning: Unclassified warning messages. Used via warn().
  *   - WarnUser: Unclassified warning messages that should be delivered even
  *     to a non-BBN user. This class should only be used for the types
  *     of warnings that a non-BBN user could respond to (e.g. a missing
  *     document date, but not a training/decode mismatch). Used via warn_user().
  *
  * ERRORS: Indicate a fatal problem.
  *   - UnexpectedInputError: An error indicating that the input document
  *     was invalid in some way that prevented SERIF from processing it.
  *     Used via err_unexpected_input(). Less common than err().
  *   - InternalInconsistencyError: An error indicating that there is a
  *     bug in SERIF. Used via err_internal_inconsistency(). Less common
  *     than err().
  *   - Error: Unclassified error messages. Used via err().
  * @see http://wiki.d4m.bbn.com/wiki/Serif/SessionLogger for additional information
  */
class SessionLogger {
public:
	// STATIC METHODS - SETTING UP/DELETING LOGGER
	// Pass in an address to a SessionLogger subclass object as logger_in. If store_old_logger
	// is true, it will internally maintain a pointer to the global logger ptr that it had
	// before the call. This can be retrieved later by calling restorePrevGlobalLogger().
	static void setGlobalLogger(SessionLogger * logger_in, bool store_old_logger=false);
	// Returns true if setGlobalLogger() has been called on a SessionLogger subclass object.
	static bool globalLoggerExists();
	// Deletes the object pointed to by the global logger ptr (i.e., the one previously
	// passed to setGlobalLogger()).
	static void deleteGlobalLogger();
	static void printGlobalLoggerInfo(); // Useful for debugging
	/**
	 * This method does NOT delete any logger objects. To do that, the caller should call deleteGlobalLogger()
	 * before calling this method.
	 * @return True if a nonzero global logger was stored previously, false otherwise.
	 **/
	static bool restorePrevGlobalLogger();

	// Used by a client to unregister the logger; client must then take care of deleting logger object separately
	// (unless it has already cleaned itself up).
	static void unsetGlobalLogger();

	// NON-STATIC METHODS
	SessionLogger(const std::vector<std::wstring> & context_level_names, const wchar_t * output_prefix=0);
    // slightly less convenient than the previous; offered for backwards compatibility
	SessionLogger(size_t n_context_levels, const wchar_t **context_level_names,
		const wchar_t *output_prefix=0);

	// This method does not make any ParamReader calls. It is called by the NullSessionLogger constructor.
	SessionLogger();
	virtual ~SessionLogger();

	typedef enum {
		DEBUG_LEVEL, 
		INFO_LEVEL, 
		WARNING_LEVEL, 
		USER_WARNING_LEVEL,
		UNEXPECTED_INPUT_ERROR_LEVEL,
		INTERNAL_INCONSISTENCY_ERROR_LEVEL,		
		ERROR_LEVEL
	} LogLevel;
	static const size_t NUM_LEVELS = ERROR_LEVEL+1;

	/**
	 * Object that uses an internal wostringstream to assemble a message. When the object
	 * goes out of scope, it adds a final newline and flushes the output. There is
	 * generally no need for callers to explicitly instantiate objects of this class;
	 * this is done automatically by dbg(), info(), warn(), err(), etc.
	 **/
	class LogMessageMaker {
	public:
		// Append to this log message. Note: for std::endl, use "\n" (but note that one will be appended
		// to the end of the message anyway). If you need to do more complicated stream manipulation,
		// perform it on an ostringstream object, and when you're finished, use its str() method to obtain
		// an output string that can be fed to a LogMessageMaker.
		LogMessageMaker &operator<<(const char *s);
		LogMessageMaker &operator<<(const wchar_t *s);
		LogMessageMaker &operator<<(const std::string &s);
		LogMessageMaker &operator<<(const std::wstring &s);
		LogMessageMaker &operator<<(bool b);
		LogMessageMaker &operator<<(char c);
		LogMessageMaker &operator<<(short i);
		LogMessageMaker &operator<<(int i);
		LogMessageMaker &operator<<(long i);
		LogMessageMaker &operator<<(unsigned short i);
		LogMessageMaker &operator<<(unsigned int i);
		LogMessageMaker &operator<<(unsigned long i);
		LogMessageMaker &operator<<(unsigned long long i);
		LogMessageMaker &operator<<(double r);
		LogMessageMaker &operator<<(const UnrecoverableException &);
		LogMessageMaker &operator<<(std::ostream&(*f)(std::ostream&)); // for std::endl (causes it to be ignored)
		LogMessageMaker &operator<<(const std::ostringstream &ostr);
		LogMessageMaker &operator<<(const std::wostringstream &ostr);
		LogMessageMaker &operator<<(const Symbol & s);
		LogMessageMaker &operator<<(const boost::gregorian::date &);

		/** Set this log message's id.  The string log_message_id is required to outlive
		 * the LogMessageMaker -- typically, you should use a string constant. Note that dbg(), info(), warn(),
		 * err(), and similar methods all perform with_id() automatically. */
		LogMessageMaker &with_id(const char* log_message_id);
        /** If stream_id is non-null, it will tell the SessionLogger to write to the stream 
         * whose ID has been set to that string. If no stream has had that ID assigned, 
         * the stream_id will be ignored. See code for details. */
		LogMessageMaker &with_stream_id(const char* log_message_id);
		/** For example, SessionLogger::info("some_msg_id").add_trail_newline(false) << "Hello world"; will prevent a
		 * final newline from being added (unless it's present in the message itself, as in "Hello world\n").
		 * By default, newlines are added, so add_trail_newline(true) is unnecessary. */
		LogMessageMaker &add_trail_newline(bool add_newline);
		/** Destructor: when this is called, the message gets written. */
		~LogMessageMaker();
		/** Copy constructor. */
		LogMessageMaker(const LogMessageMaker &other);
	private:
		friend class SessionLogger;
		LogMessageMaker(LogLevel level);
		boost::shared_ptr<std::wostringstream> _msg;
		const char* _identifier;
		const char* _stream_id;
		LogLevel _level;
		LogMessageMaker& operator=(const LogMessageMaker &other); // intentionally not defined.
		bool _add_trail_newline;
	};
public:
    // CORE METHODS - STATIC
    // These presume that the caller has already initialized SessionLogger::logger.


	// We expect these four methods (which have short names) to be used frequently.
	/**
	 * Writes a message that will be printed if the log level has been set (via the "log_threshold" param read by ParamReader) 
	 * to DEBUG_LEVEL or higher.
     * Messages can be suppressed by including their identifiers in the value for the "log_ignore" param read by ParamReader. 
	 * Their display can be forced by including their identifiers in the value for the "log_force" param read by ParamReader.
	 * @param log_message_id A string containing an identifier (which need not be unique) for the message.
	 * @param stream_id An optional string whose value is used to indicate the output stream for SessionLogger objects that
	 * can have more than one output stream. For example, setting this value to "std::wcout", "std:cout", "wcout", or "cout" 
	 * will cause ConsoleSessionLogger to write the message to cout rather than the default cerr.
     * @example SessionLogger::dbg("patt_dir") << "Reading pattern dir " << dirname << "\n";
	 * @author afrankel@bbn.com
	 **/
	static LogMessageMaker dbg(const char* log_message_id, const char* stream_id=NULL);

	/** Writes a message that will be printed if the log level has been set to INFO_LEVEL or higher. See dbg() for details. */
	static LogMessageMaker info(const char* log_message_id, const char* stream_id=NULL);

	/** Writes a message that will be printed if the log level has been set to WARN_LEVEL or higher. See dbg() for details. */
	static LogMessageMaker warn(const char* log_message_id, const char* stream_id=NULL);

	/** Writes a message that will be printed if the log level has been set to WARN_USER_LEVEL or higher. See dbg() for details. */
	static LogMessageMaker warn_user(const char* log_message_id, const char* stream_id=NULL);

	/** Writes a message that will be printed if the log level has been set to ERR_LEVEL or higher. See dbg() for details. */
	static LogMessageMaker err(const char* log_message_id, const char* stream_id=NULL);

	/** More specific classes of error messages */
	static LogMessageMaker err_unexpected_input(const char* log_message_id, const char* stream_id=NULL);
	static LogMessageMaker err_internal_inconsistency(const char* log_message_id, const char* stream_id=NULL);

	static void updateContext(size_t context_level, const wchar_t *context_info);
	static void updateContext(size_t context_level, const char *context_info);

	/**
	 * If this method returns false, the corresponding message will not be displayed, either because
	 * (a) we're running at a higher log level than DEBUG_LEVEL (and we're not forcing display of that particular
	 * message via the "log_force" parameter) or (b) that message is indicated, via the "log_ignore" parameter, 
	 * as one that is to be suppressed. You can use this to skip intensive computation required to produce
	 * a message that would be swallowed anyway.
	 * @example if (SessionLogger::dbg_or_msg_enabled("patt_dir") {SessionLogger::dbg("patt_dir") << obj.toDebugString();}
	 * @author afrankel@bbn.com
	 **/
	static bool dbg_or_msg_enabled(const char* identifier);

	/** Similar to enabled_for_dbg(), with DEBUG_LEVEL changed to INFO_LEVEL. */
	static bool info_or_msg_enabled(const char* identifier);

	/** Similar to enabled_for_dbg(), with DEBUG_LEVEL changed to WARN_LEVEL. */
	static bool warn_or_msg_enabled(const char* identifier);

	// We expect this to be used infrequently, if at all, as the user will probably always want to see error messages.
	static bool err_or_msg_enabled(const char* identifier);

	// We expect these to be used very infrequently, if at all.
	static bool user_warning_or_msg_enabled(const char* identifier);
	static bool err_unexpected_input_or_msg_enabled(const char* identifier);
	static bool err_internal_inconsistency_or_msg_enabled(const char* identifier);

	// We expect this to be used infrequently outside this class, if at all.
	static bool enabledAtLevel(const char* identifier, LogLevel level);

	// NON-STATIC VERSIONS OF STATIC METHODS
	void updateLocalContext(size_t context_level, const wchar_t *context_info);
	void updateLocalContext(size_t context_level, const char *context_info);
	bool enabledAtLocalLevel(const char* identifier, LogLevel level) const;

	/** Print message to console indicating how things went */
	virtual void displaySummary();

	virtual void displayString(const std::string &msg, const char *stream_id);

	// Methods that should be implemented by the subclasses.
	virtual void displayString(const std::wstring &msg, const char *stream_id) = 0;
	virtual void flush() {}
	virtual std::string getType(); // Useful for debugging

public:
	// NON-STATIC
	/** output another piece of the message */
	SessionLogger &operator<<(const std::wstring &s);
	SessionLogger &operator<<(const char *s);
	SessionLogger &operator<<(const wchar_t *s);
	SessionLogger &operator<<(const std::string &s);
	SessionLogger &operator<<(bool b);
	SessionLogger &operator<<(char c);
	SessionLogger &operator<<(short i);
	SessionLogger &operator<<(int i);
	SessionLogger &operator<<(long i);
	SessionLogger &operator<<(unsigned short i);
	SessionLogger &operator<<(unsigned int i);
	SessionLogger &operator<<(unsigned long i);
	SessionLogger &operator<<(unsigned long long i);
	SessionLogger &operator<<(double r);
	SessionLogger &operator<<(const UnrecoverableException& exc);
	SessionLogger &operator<<(std::ostream&(*f)(std::ostream&)); // for std::endl (causes it to be ignored)
	SessionLogger &operator<<(const std::ostringstream& ostr); 
	SessionLogger &operator<<(const std::wostringstream& ostr); 
	SessionLogger &operator<<(const Symbol &s);
	SessionLogger &operator<<(const boost::gregorian::date &s);

	LogMessageMaker reportDebugMessage() { return LogMessageMaker(DEBUG_LEVEL); }
	LogMessageMaker reportInfoMessage() { return LogMessageMaker(INFO_LEVEL); }
	LogMessageMaker reportWarning() { return LogMessageMaker(WARNING_LEVEL); }
	LogMessageMaker reportUserWarning() { return LogMessageMaker(USER_WARNING_LEVEL); }
	LogMessageMaker reportUnexpectedInputError() { return LogMessageMaker(UNEXPECTED_INPUT_ERROR_LEVEL); }
	LogMessageMaker reportInternalInconsistencyError() { return LogMessageMaker(INTERNAL_INCONSISTENCY_ERROR_LEVEL); }
	LogMessageMaker reportError() { return LogMessageMaker(ERROR_LEVEL); }
	void reportMessage(LogLevel level, const char* identifier, const char* stream_id, const std::wstring& msg,
		bool add_newline);
	/**
	 * Can be called by a class that has multiple streams, like ConsoleSessionLogger, with wcerr and wcout, or
	 * TeeSessionLogger, with wcerr, wcout, and the file output, to indicate a name corresponding to a given stream.
	 * Messages that specify a stream id will then be written to that stream. Messages that do not specify a stream id
	 * will be written to the default stream.
	 * @author afrankel@bbn.com
	 **/
	void setStreamId(const char* stream_id, std::ostream & ostr);
private:
	struct Internals;
	Internals *_impl;

	bool isMessageSuppressed(const char* identifier) const;
	bool isMessageForced(const char* identifier) const;
	void writeContext();

	// STATIC METHODS
	static SessionLogger * getLogger();

    // STATIC DATA MEMBERS
	static SessionLogger *_globalLoggerPtr;
	static SessionLogger *_prevGlobalLoggerPtr;
	//static NullSessionLogger _defaultLogger;
protected:
	// We expect these to be used only by subclasses that can write to different output streams.
	/** Returns true for "std::wcerr", "std:cerr", "wcerr", or "cerr". */
	static bool is_wcerr_synonym(const char * stream_id);
	/** Returns true for "std::wcout", "std:cout", "wcout", or "cout". */
	static bool is_wcout_synonym(const char * stream_id);

public:
	// Don't use this except within SessionLogger.cpp! It's only public in order to resolve a 
	// backwards compatibility issue.
	static ConsoleSessionLogger _defaultLogger;


public:
	// EVERYTHING IN THIS SECTION IS DEPRECATED - FOR BACKWARDS COMPATIBILITY ONLY

/** This is the old interface for using the SessionLogger.  To
  * use this deprecated interface, first call beginMessage(),
  * beginEvent(), beginWarning(), or beginError(). Then use << to send
  * info about the message to the log. What actually gets output will
  * include context information (unless you used beginMessage()).
  */
	/** Get ready to print any message without context */
	void beginMessage();
	/** Get ready to print friendly message, with context */
	void beginEvent();
	/** Get ready to print bad message, with context */
	void beginWarning();
	/** Get ready to print really bad (showstopping) message, with context */
	void beginError();

	static SessionLogger * logger;


};


#endif
