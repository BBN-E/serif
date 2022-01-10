// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/common/HeapStatus.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/SessionLogger.h"
#include "Generic/common/DebugStream.h"
#include <string>

#ifndef _WIN32
#include <unistd.h>
#endif

bool HeapStatus::_active = true;

using namespace std;

size_t HeapStatus::getHeapSize() {
	if (!_active)
		return 0;
#if defined(_WIN32)
	size_t size = 0;
	_HEAPINFO hinf;
	int status;
	hinf._pentry = NULL;
	while ((status = _heapwalk(&hinf)) == _HEAPOK) {
		if (hinf._useflag == _USEDENTRY)
			size += hinf._size;
	}
	if (status == _HEAPEMPTY || status == _HEAPEND)
		return size;
	throw InternalInconsistencyException("HeapStatus::getHeapSize", "bad heap!");
#else
	// In linux, we look up our total memory usage, not just heap usage:
	ifstream proc_status("/proc/self/statm");
	if (proc_status.good()) {
	    size_t num_pages = 0;
	    proc_status >> num_pages;
	    proc_status.close();
	    return num_pages * getpagesize();
	} else {
	    return 0; // Unknown.
	}
#endif
}

void HeapStatus::takeReading(std::string s) {
	takeReading(s.c_str());
}

void HeapStatus::takeReading(const char* s) {
	if (!_active)
		return;
	if (_reading_size >= MAX_READINGS) {
		// Discard an old reading to make room for the new one (but always
		// keep the first reading, since it's probably a baseline).
		for (size_t i=2; i<MAX_READINGS; ++i) {
			_readings[i-1] = _readings[i];
			strcpy(_messages[i-1], _messages[i]);
		}
		--_reading_size;
	}
	_readings[_reading_size] = getHeapSize();
	if (_readings[_reading_size] > _max_reading)
		_max_reading = _readings[_reading_size];
	if ((_min_reading==0) || (_readings[_reading_size] < _min_reading))
		_min_reading = _readings[_reading_size];

	size_t sLength = strlen(s);
	if (sLength > MAX_MSG_LENGTH-1) {
		SessionLogger::warn("heap") << "Heap status message too long! It will be truncated\n";
	}
	// terminate string with \0 if the space is overflowed
	if (_snprintf(_messages[_reading_size], MAX_MSG_LENGTH, "%s", s) < 0)
		_messages[_reading_size][MAX_MSG_LENGTH-1] = 0;
	_reading_size++;
}


void HeapStatus::flushReadings()
{
	_reading_size = 0;
}

void HeapStatus::displayReadings(DebugStream& strm)
{
	if (_active)
		strm << getReadingReport().c_str();
}

void HeapStatus::displayReadings()
{
	if (_active)
		SessionLogger::info("diff_0") << getReadingReport();
}

void HeapStatus::printReadings() {
	if (_active)
		std::cout << getReadingReport().c_str();
}

std::string HeapStatus::getReadingReport() {
	ostringstream ostr;
	size_t i;
	for (i=0; i < _reading_size; i++) {
		ostr << "\t[memory] ";
		ostr << _messages[i];
		ostr << ":  ";
		ostr << _readings[i];
		if (i > 0) {
			ostr << "  (";
			ostr << (static_cast<int>(_readings[i]) - 
					 static_cast<int>(_readings[i-1]));
			ostr << ")";
		}
		ostr << "\n";
	}
	if (_reading_size > 2) {
		ostr << "[memory] TOTAL DIFFERENCE: ";
		ostr << _readings[_reading_size-1]-_readings[0];
		ostr << "\n";
	}
	ostr << "\n[memory] MIN MEMORY READING: " << _min_reading;
	ostr << "\n[memory] MAX MEMORY READING: " << _max_reading;
	return ostr.str();
}
