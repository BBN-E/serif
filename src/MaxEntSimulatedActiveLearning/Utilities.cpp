// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "common/leak_detection.h"

#include "Utilities.h"
#include <boost/scoped_ptr.hpp>


bool validMention(Mention* m) {
	if (!m->isOfRecognizedType() || 
		m->getMentionType() == Mention::NONE ||
		m->getMentionType() == Mention::APPO ||
		m->getMentionType() == Mention::LIST)

		return false;
	return true;
}

int countStateLinesInFile(const wchar_t *const filename) {
	UTF8Token token;
	int num_sentences = 0;
	Symbol serifStateSym(L"Serif-state");
	boost::scoped_ptr<UTF8InputStream> file_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& file(*file_scoped_ptr);
	while (!file.eof()) {
		file >> token;
		if (token.symValue() == serifStateSym)
			num_sentences++;
	}
	file.close();
	return num_sentences;
}
int countLinesInFile(const wchar_t *const filename) {
	UTF8Token token;
	int numLines = 0;
	boost::scoped_ptr<UTF8InputStream> file_scoped_ptr(UTF8InputStream::build(filename));
	UTF8InputStream& file(*file_scoped_ptr);
	while (!file.eof()) {
		file >> token;
		numLines++;
	}
	file.close();
	return numLines;
}


void getWideStringParam(const char* const paramName, wchar_t* buffer, int bufferSize, const char* const source) {
	wchar_t wideParamName[MAXLINE];
	mbstowcs(wideParamName, paramName, MAXLINE);
	Symbol param = ParamReader::getParam(Symbol(wideParamName));
	if (param == Symbol()) {
		char error[MAXLINE];
		sprintf(error, "Parameter '%s' not specified", paramName);  // XXX overflow possibility
		throw UnexpectedInputException(source, error);
	}
	wcsncpy(buffer, param.to_string(), bufferSize);
}

void getStringParam(const char* const paramName, char* buffer, int bufferSize, const char* const source) {
	if (!ParamReader::getParam(paramName,buffer,bufferSize)) {
		char error[MAXLINE];
		sprintf(error, "Parameter '%s' not specified", paramName);  // XXX overflow possibility
		throw UnexpectedInputException(source, error);
	}
}

int getIntParam(const char* const paramName, const char* const source) {

	char param[MAXLINE];
	if (!ParamReader::getParam(paramName,param,MAXLINE)) {
		char error[MAXLINE];
		sprintf(error, "Parameter '%s' not specified", paramName);  // XXX overflow possibility
		throw UnexpectedInputException(source, error);
	}
	return atoi(param);
}

double getDoubleParam(const char* const paramName, const char* const source) {
	char param[MAXLINE];
	if (!ParamReader::getParam(paramName,param,MAXLINE)) {
		char error[MAXLINE];
		sprintf(error, "Parameter '%s' not specified", paramName);  // XXX overflow possibility
		throw UnexpectedInputException(source, error);
	}
	return atof(param);
}
