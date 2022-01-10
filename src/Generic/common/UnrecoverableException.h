// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef UNRECOVERABLE_EXCEPTION_H
#define UNRECOVERABLE_EXCEPTION_H

#ifdef _WIN32
#pragma warning(disable: 4290)
#endif

//#include "string.h"
#include <iostream>
#include <sstream>
#include <string>

#include "Generic/linuxPort/serif_port.h"

class UnrecoverableException {
public:
	static const int MAX_STR_LEN = 8191;

	// Source is the fully-qualified name of function from which the exception
	// was thrown, e.g., "Symbol::to_debug_string()".
	UnrecoverableException(const char *source,
						   const char *message = "Unknown error")
	{
		strncpy_s(_source, source, MAX_STR_LEN);
		strncpy_s(_message, message, MAX_STR_LEN);
		_is_logged = false;
	}
	UnrecoverableException(const char *source, const std::string& string) {
        const char* message = string.c_str();
        strncpy_s(_source, source, MAX_STR_LEN);
        strncpy_s(_message, message, MAX_STR_LEN);
		_is_logged = false;
    }
	UnrecoverableException(const char *source, const std::stringstream& stringStream) {
        const char* message = stringStream.str().c_str();
        strncpy_s(_source, source, MAX_STR_LEN);
        strncpy_s(_message, message, MAX_STR_LEN);
		_is_logged = false;
    }

	// Default constructor -- used by subclasses
	UnrecoverableException() {
		strcpy_s(_source, "Unspecified");
		strcpy_s(_message, "Unspecified -- definitely a bug here, sorry...");
		_is_logged = false;
	}

	virtual ~UnrecoverableException() {}


	const char *getSource() const {
		return _source;
	}

	const char *getMessage() const {
		return _message;
	}

	bool isLogged() const {
		return _is_logged;
	}

	void markAsLogged() {
		_is_logged = true;
	}

	void prependToMessage(const char *s) {
		char temp[MAX_STR_LEN];
		strncpy_s(temp, s, MAX_STR_LEN / 2);
		strncat_s(temp, _message, MAX_STR_LEN / 2);
		strncpy_s(_message, temp, MAX_STR_LEN);
	}


	void putMessage(std::ostream& out) const {
		out << "Unrecoverable error: " << _source << ": " << _message;
	}

	friend std::ostream &operator<<(std::ostream &o, UnrecoverableException &e) {
		e.putMessage(o);
		return o;
	}

protected:
	char _source[MAX_STR_LEN+1];
	char _message[MAX_STR_LEN+1];
	bool _is_logged;
};


#endif
