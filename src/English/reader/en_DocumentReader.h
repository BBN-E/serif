// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_DOCUMENT_READER_H
#define EN_DOCUMENT_READER_H

#include "Generic/theories/Document.h"
#include "Generic/reader/DocumentReader.h"
#include "Generic/common/UTF8InputStream.h"

#define MAX_DOC_REGIONS 100

class EnglishDocumentReader: public DocumentReader {
private:
	friend class EnglishDocumentReaderFactory;

public:


	/**
	  * Reads from strm until it reaches the end of 
	  * file and returns a pointer to a document object
	  * containing the located raw text of the file, 
	  * ignoring only text surrounded by '<' and '>'.
	  * The client is responsible for deleting the
	  * document.
	  *
	  * @param stream the input stream to read from.
	  */
	virtual Document* readDocument(InputStream &stream);
	virtual Document* readDocument(InputStream &strm, const wchar_t * filename) { return 0; }
	virtual ~EnglishDocumentReader();

protected:
	static LocatedString *_regions[MAX_DOC_REGIONS];

	/// Removes annotations and unrecognized tags and translates SGML entities.
	virtual void cleanRegion(LocatedString *region);
	Symbol _inputType;

	/// Turns raw text into something that can be processed by Serif
	void processRawText(LocatedString *document);
	int _rawtext_doc_count;

private:
	EnglishDocumentReader();
};

class EnglishDocumentReaderFactory: public DocumentReader::Factory {
	virtual DocumentReader *build() { return _new EnglishDocumentReader(); }
};


#endif
