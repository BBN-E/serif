// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef BATCH_H
#define BATCH_H

#include <string>
#include <vector>


/** Just a quaint little structure for holding lists of document names */

class Batch {
public:
	Batch() : _doc_index(0) {}
	~Batch();

	void loadFromFile(const char *batch_file);
	void loadFromSegmentFile(const char *batch_file);
	void addDocument(const std::wstring &path);

	// Old interface (requires non-const batch):
	void resetToFirstDocument();
	const wchar_t *getNextDocumentPath();
	bool hasMoreDocuments();

	// New interface (allows for const batch):
	size_t getNDocuments() const { return _documentPaths.size(); }
	const wchar_t *getDocumentPath(size_t i) const;
private:

	std::vector<std::wstring> _documentPaths;

	/** Index into _filenames of the next document path that we should return. */
	size_t _doc_index;
};


#endif
