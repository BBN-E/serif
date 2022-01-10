// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef REGION_DOCUMENT_SPLITTER_H
#define REGION_DOCUMENT_SPLITTER_H

#include "Generic/reader/DocumentSplitter.h"

#ifndef SERIF_EXPORTED
#define SERIF_EXPORTED
#endif

typedef std::pair<int, int> RegionBound;

// Wrapper so we can track whether a Region was created or came from a Document
typedef std::pair<Region*, bool> RegionSource;

class SERIF_EXPORTED RegionDocumentSplitter : public DocumentSplitter {
public:
	// Override the constructor to read the threshold parameter
	RegionDocumentSplitter();

	// If no split is necessary doesn't delete input, caller is responsible; otherwise deletes input after splitting
 	virtual std::vector<Document*> splitDocument(Document* document);

	// If only one document, pass through; otherwise delete input after merging
	virtual DocTheory* mergeDocTheories(std::vector<DocTheory*> splitDocTheories);

protected:
	// Helper method for additional splitting inside a region
	void splitRegion(const Region* region, std::vector<RegionSource> &regions);

private:
	// Control the threshold at which we break regions into groups, or break up large regions
	int _max_document_chars;
};

#endif
