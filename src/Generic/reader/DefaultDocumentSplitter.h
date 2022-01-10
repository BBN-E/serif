// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DEFAULT_DOCUMENT_SPLITTER_H
#define DEFAULT_DOCUMENT_SPLITTER_H

#include "Generic/reader/DocumentSplitter.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

class SERIF_EXPORTED DefaultDocumentSplitter : public DocumentSplitter {
public:
	// Wraps the input document as the only split member; doesn't delete input, caller is responsible
 	virtual std::vector<Document*> splitDocument(Document* document);

	// Returns the only input document; caller is responsible for deleting
	virtual DocTheory* mergeDocTheories(std::vector<DocTheory*> splitDocTheories);
};

#endif
