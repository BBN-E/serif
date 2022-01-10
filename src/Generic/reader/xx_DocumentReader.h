// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_DOCUMENT_READER_H
#define xx_DOCUMENT_READER_H

#include "Generic/reader/DocumentReader.h"
#include "Generic/reader/DefaultDocumentReader.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED GenericDocumentReader : public DocumentReader {
private:
	friend class GenericDocumentReaderFactory;

public:	
	virtual Document* readDocument(InputStream &strm, const wchar_t * filename) {
		return _documentReader.readDocument(strm, filename);
	}

private:
	GenericDocumentReader() {}

	DefaultDocumentReader _documentReader;
};

class GenericDocumentReaderFactory: public DocumentReader::Factory {
	virtual DocumentReader *build() { return _new GenericDocumentReader(); }
};

#endif


