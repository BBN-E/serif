// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DUMMY_REFERENCE_RESOLVER_H
#define DUMMY_REFERENCE_RESOLVER_H

#include "Generic/theories/DocTheory.h"
#include "Generic/theories/MentionSet.h"
#include "Generic/theories/EntitySet.h"
#include "Generic/common/GrowableArray.h"


class DummyReferenceResolver {
public:
	DummyReferenceResolver() : _lastEntitySet(0) {}
	~DummyReferenceResolver() {}
	void resetForNewDocument(DocTheory *docTheory) {
		_nSentences = docTheory->getNSentences();
	}
	void resetForNewSentence(DocTheory *docTheory, int sentence_num) {
		if (sentence_num == 0) {
			_lastEntitySet = 0;
        } else {
			_lastEntitySet = docTheory->getSentenceTheory(sentence_num-1)->getEntitySet();
		}
	}

	EntitySet *createDefaultEntityTheory(MentionSet *mentionSet) {
		EntitySet *eset;
		// use 'false' with _new EntitySet so that it doesn't make a copy of its
		//  last mention set -- each EntitySet should only own one unique MentionSet,
		//  which lives in its _currMentionSet pointer. eset's _currMentionSet will
		//  be created as a copy of mentionSet when you call loadMentionSet
		if (_lastEntitySet != 0)
			eset = _new EntitySet(*_lastEntitySet, false);
		else eset = _new EntitySet(_nSentences);
		eset->loadMentionSet(mentionSet);
		return eset;
	}

private:
	EntitySet *_lastEntitySet;
	int _nSentences;
	
};

#endif
