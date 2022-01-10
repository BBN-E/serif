// Copyright 2011 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PATTERN_NAME_FINDER_H
#define PATTERN_NAME_FINDER_H

class DocTheory;
class PIdFSentence;
class DTTagSet;

#include "Generic/patterns/PatternSet.h"

class PatternNameFinder {

public:
	PatternNameFinder();
	~PatternNameFinder();

	void resetForNewSentence(DocTheory *docTheory, int sentence_num);
	void augmentPIdFSentence(PIdFSentence &sentence, DTTagSet *tagSet);

private:
	DocTheory *_docTheory;
	int _sentence_number;
	std::vector<PatternSet_ptr> _patternSets;

};

#endif
