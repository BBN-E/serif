// Copyright 2016 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PROSE_DOCUMENT_ZONER_H
#define PROSE_DOCUMENT_ZONER_H

#include "Generic/reader/DocumentZoner.h"
#include "Generic/theories/Region.h"

#include <vector>

class  ProseDocumentZoner : public DocumentZoner {
public:
	ProseDocumentZoner();

	// Override the process method
	virtual void process(DocTheory* docTheory);

	static const Symbol PROSE_TAG;
	static const Symbol NON_PROSE_TAG;

private:
	const size_t WINDOW_LENGTH;
	const double THRESHOLD;
	const size_t MIN_NON_PROSE_SECTION_LENGTH;

	enum Classification { PROSE, NON_PROSE, UNKNOWN };
	typedef std::pair<int, int> OffsetPair;
	
	struct ClassifiedWord {
		int start_offset;
		int end_offset;
		Classification classification;

		ClassifiedWord(int start_offset, int end_offset, Classification classification)
			: start_offset(start_offset), end_offset(end_offset), classification(classification) { }
	};

	void processRegion(Document *document, const Region *r, std::vector<Region *> &newRegions);
	Classification classifyWord(const LocatedString *contents, int start_offset, int end_offset);
	bool isJunkyToken(std::wstring word);
	bool isHeaderToken(std::wstring word, const LocatedString *contents, int start_offset, int end_offset);
	int getIndexOfLastNonProse(std::vector<ClassifiedWord> &words, size_t start_index);
};

#endif
