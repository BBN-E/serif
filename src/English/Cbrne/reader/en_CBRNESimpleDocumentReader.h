// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_CBRNESIMPLEDOCREADER_H
#define en_CBRNESIMPLEDOCREADER_H

#include "Generic/theories/Document.h"
#include "English/reader/en_DocumentReader.h"
#include "Generic/reader/DefaultDocumentReader.h"
#include "Generic/common/InputStream.h"
#include "Generic/common/limits.h"



class CBRNESimpleDocumentReader: public DefaultDocumentReader {
public:

	CBRNESimpleDocumentReader();

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
	virtual class Document* readDocument(InputStream &stream);
	virtual Document* readDocument(InputStream &stream, const wchar_t * filename) { return DefaultDocumentReader::readDocument(stream, filename); }
	virtual Document* readDocument(const LocatedString *source, const wchar_t * filename) { return DefaultDocumentReader::readDocument(source, filename); }
	virtual Document* readDocument(const LocatedString *source, const wchar_t * filename, Symbol inputType) { return DefaultDocumentReader::readDocument(source, filename, inputType); }
	virtual Document* readDocument(InputStream &stream, const wchar_t * filename, Symbol inputType) { return DefaultDocumentReader::readDocument(stream, filename, inputType); }
	virtual ~CBRNESimpleDocumentReader();

protected:

	virtual void cleanRegion(LocatedString *region, Symbol inputType);

};

#endif
