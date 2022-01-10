// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PATTERN_VALUE_FINDER_H
#define PATTERN_VALUE_FINDER_H

class DocTheory;
class DTTagSet;
class SentenceTheory;

#include "Generic/patterns/PatternSet.h"
#include "Generic/values/ValueRecognizer.h"

class PatternValueFinder {

public:
	PatternValueFinder();
	~PatternValueFinder();

	void resetForNewDocument(DocTheory *docTheory);
	ValueRecognizer::SpanList findValueMentions(int sent_no);

private:
	DocTheory *_docTheory;
	std::vector<PatternSet_ptr> _patternSets;

};

#endif
