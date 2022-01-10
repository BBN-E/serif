// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/driver/Batch.h"
#include "Generic/common/UnexpectedInputException.h"
#include "Generic/common/InternalInconsistencyException.h"
#include "Generic/common/Segment.h"
#include "Generic/common/UTF8InputStream.h"

#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/scoped_ptr.hpp>

using namespace std;


Batch::~Batch() {
}

void Batch::resetToFirstDocument() { 
	_doc_index = 0; 
}

bool Batch::hasMoreDocuments() { 
	return (_doc_index < _documentPaths.size());
}

void Batch::loadFromFile(const char *batch_file) {
	_doc_index = 0; 
	_documentPaths.clear();

	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(batch_file));
	UTF8InputStream& stream(*stream_scoped_ptr);
	std::wstring line;
	while (stream) {
		stream.getLine(line);

		// test if the extraction failed
		if (!stream) 
			break;
		
		if (line.size()>0 && line[0]!=L'#') {
			addDocument(line);
		}
	}
	stream.close();
}

void Batch::loadFromSegmentFile(const char *batch_file) {
	_doc_index = 0; 
	_documentPaths.clear();
	
	boost::scoped_ptr<UTF8InputStream> stream_scoped_ptr(UTF8InputStream::build(batch_file));
	UTF8InputStream& stream(*stream_scoped_ptr);	
	while (stream) {
		WSegment seg; stream >> seg;
		
		// test if the extraction failed
		if (!stream) 
			break;
		
		wstring w_file = seg[L"file"].at(0).value;
		addDocument(w_file);
	}
	stream.close();
}

void Batch::addDocument(std::wstring const &path) {
	_documentPaths.push_back(boost::trim_copy_if(path, boost::is_any_of(" \t\n\r")));
}

const wchar_t *Batch::getNextDocumentPath() {
	if (hasMoreDocuments()) {
		return _documentPaths[_doc_index++].c_str();
	} else {
		return 0;
	}
}

const wchar_t *Batch::getDocumentPath(size_t i) const { 
	if (i >= _documentPaths.size())
		throw InternalInconsistencyException::arrayIndexException(
			"Batch::getDocumentPath()", static_cast<int>(_documentPaths.size()), static_cast<int>(i));
	return _documentPaths[i].c_str();
}
