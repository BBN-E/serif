// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include <string>
#include "Generic/common/LocatedString.h"
#include "Generic/common/UnrecoverableException.h"
#include "Generic/common/UTF8OutputStream.h"
#include "Generic/preprocessors/ResultsCollector.h"

namespace DataPreprocessor {

/**
 * @param output_file the file to collect results in.
 */
ResultsCollector::ResultsCollector(const char *output_file)
{
	_stream = NULL;
	_string = NULL;
	open(output_file);
}

ResultsCollector::ResultsCollector()
{
	_stream = NULL;
	_string = _new std::wstring(L"");
}

ResultsCollector::~ResultsCollector()
{
	if (_stream != NULL) {
		_stream->flush();
		_stream->close();
		delete _stream;
	}
	if (_string != NULL) {
		delete _string;
	}
}

/**
 * If the results collector is already opened on an output file,
 * this function has no effect, i.e., it does not close the current
 * file or open the given file. The file is automatically flushed
 * and closed when this object is destroyed.
 *
 * @param output_file the file to open.
 */
void ResultsCollector::open(const char *output_file)
{
	if (_string != NULL) {
		delete _string;
		_string = NULL;
	}
	if (_stream == NULL) {
		_stream = _new UTF8OutputStream(output_file);
	}
}

/**
 * @return the collected results.
 * @throws UnrecoverableException if results were collected in a file
 *                                rather than an internal buffer.
 */
const wchar_t *ResultsCollector::getResults()
{
	if (_string == NULL) {
		throw UnrecoverableException("ResultsCollector::getResults()", "results written to file");
	}
	return _string->c_str();
}

/**
 * @param str the results to collect.
 * @return a reference to this object.
 */
ResultsCollector& ResultsCollector::operator<< (const std::wstring& str)
{
	if (_stream == NULL) {
		_string->append(str);
	}
	else {
		(*_stream) << str;
	}
	return *this;
}

/**
 * @param str the results to collect.
 * @return a reference to this object.
 */
ResultsCollector& ResultsCollector::operator<< (const wchar_t* str)
{
	if (_stream == NULL) {
		_string->append(str);
	}
	else {
		(*_stream) << str;
	}
	return *this;
}

/**
 * @param str the results to collect.
 * @return a reference to this object.
 */
ResultsCollector& ResultsCollector::operator<< (const LocatedString& str)
{
	if (_stream == NULL) {
		_string->append(str.toString());
	}
	else {
		(*_stream) << str.toString();
	}
	return *this;
}

} // namespace DataPreprocessor
