// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEBUGSTREAM_H
#define DEBUGSTREAM_H

#include "Generic/common/UTF8OutputStream.h"
#include "Generic/common/Symbol.h"

class DebugStream { 
public:
	//static DebugStream nullStream;
	static DebugStream referenceResolverStream;

	DebugStream();
	DebugStream(Symbol param_name, bool echo_to_console = false, bool append = false);
	//DebugStream(Symbol param_name, bool echo_to_console);
	~DebugStream();
	void init(Symbol param_name, bool echo_to_console = false, bool append = false);
	bool openFile(const char *file, bool echo_to_console = false, bool append = false);


	void setActive(bool active) { _active = active; }
	void setEcho(bool echo) { _echo_to_console = echo; }

	void setIndent(int indent) { _indent = indent; }
	int getIndent() { return _indent; }

	void flush() { if (_active) _out.flush(); }
	DebugStream& operator<< (const std::wstring& str);
	DebugStream& operator<< (const wchar_t* str);
	DebugStream& operator<< (const char* str);
	DebugStream& operator<< (int i);
	DebugStream& operator<< (size_t i)
		{ return *this << static_cast<int>(i); }
	DebugStream& operator<< (double n);
	
	/**
	 * Determine the active state. Because sometimes the decision
	 * to output or not can mean invoking a time consuming loop
	 * @return true if writing to this stream does something
	 */
	bool isActive() { return _active; }

private:

	UTF8OutputStream _out;
	bool _active;
	bool _echo_to_console;
	int _indent;
};

#endif
