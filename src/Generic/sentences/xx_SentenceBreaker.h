// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef xx_SENTENCE_BREAKER_H
#define xx_SENTENCE_BREAKER_H

#include "Generic/sentences/SentenceBreaker.h"

/** A sentence breaker that returns a sentence for each region.
 * This is used when the parameter do_sentence_breaking is 
 * set to false. */
class SERIF_EXPORTED NullSentenceBreaker : public SentenceBreaker {
public:
	virtual void resetForNewDocument(const Document *doc);
	virtual int getSentencesRaw(Sentence **results, int max_sentences,
			const Region* const* regions, int num_regions);
	//virtual int getSentencesRaw(Sentence **results, int max_sentences,
		//	const Zone* const* zones, int num_zones);
private:
	const Document *_curDoc;
};

#endif

