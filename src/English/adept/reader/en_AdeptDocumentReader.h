// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_ADEPTDOCREADER_H
#define en_ADEPTDOCREADER_H

#include "Generic/theories/Document.h"
#include "English/reader/en_DocumentReader.h"
#include "Generic/common/InputStream.h"

#define MAX_DOC_REGIONS 100

class AdeptDocumentReader: public EnglishDocumentReader {
public:

	AdeptDocumentReader();

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
	virtual ~AdeptDocumentReader();

protected:

	/// Removes annotations and unrecognized tags and translates SGML entities.
	virtual void cleanRegion(LocatedString *region);

};

#endif
