// Copyright 2009 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/common/leak_detection.h"
#include "Generic/sentences/xx_SentenceBreaker.h"
#include "Generic/theories/Region.h"
#include "Generic/theories/Zone.h"
#include "Generic/theories/Sentence.h"

void NullSentenceBreaker::resetForNewDocument(const Document *doc) {
	_curDoc = doc;
}

int NullSentenceBreaker::getSentencesRaw(Sentence **results, int max_sentences,
										 const Region* const* regions, int num_regions) 
{
	int n_results = 0;
	if (max_sentences > num_regions)
		max_sentences = num_regions;
	for (int i = 0; i < max_sentences; i++) {
		const Region *curRegion = regions[i];
		if (curRegion->getString()->length() != 0) 
			results[n_results++] = _new Sentence(_curDoc, curRegion, i, curRegion->getString());
	}
	return n_results;
}



