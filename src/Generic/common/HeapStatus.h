// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEAP_STATUS_H
#define HEAP_STATUS_H

#include "Generic/common/DebugStream.h"
#include <string>

#define MAX_READINGS 20
#define MAX_MSG_LENGTH 200
// stores heap size readings and messages regarding 
// those readings, and can print them out for debugging purposes
class HeapStatus {
public:
	HeapStatus() : _reading_size(0), _max_reading(0), _min_reading(0) {}
	~HeapStatus() {}

	/**
	 * toggle the functionality of this class - when not in use
	 * turning it off saves tremendous time
	 */
	static void makeActive() { _active = true; }
	static void makeInactive() { _active = false; }
	/**
	 * walk the heap and determine how many bytes are being actively used
	 */
	static size_t getHeapSize();

	/**
	 * Perform a getHeapSize and store a corresponding message.
	 */
	void takeReading(std::string s);

	void takeReading(const char* s);
	/**
	 * remove the buffered heap entries from tables
	 */
	void flushReadings();

	/**
	 * Print out all readings so far to sessionlogger or debugstream.
	 */
	void displayReadings(DebugStream& strm);

	// display to session logger
	void displayReadings();

	// display to stdout
	void printReadings();

private:
	static bool _active;
	char _messages[MAX_READINGS][MAX_MSG_LENGTH];
	size_t _readings[MAX_READINGS];
	size_t _reading_size;
	size_t _max_reading;
	size_t _min_reading;

	std::string getReadingReport();

};

#endif
