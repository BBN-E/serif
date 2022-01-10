// Copyright 2013 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"

#include "Generic/reader/DefaultDocumentSplitter.h"

std::vector<Document*> DefaultDocumentSplitter::splitDocument(Document* document) {
	std::vector<Document*> split;
	split.push_back(document);
	return split;
}

DocTheory* DefaultDocumentSplitter::mergeDocTheories(std::vector<DocTheory*> splitDocTheories) {
	if (splitDocTheories.size() != 1)
		throw InternalInconsistencyException("DefaultDocumentSplitter::mergeDocuments", "Expected one and only one document for no-op default merge");
	return splitDocTheories[0];
}
