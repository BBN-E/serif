// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PRONOUN_CLASSIFIER_H
#define EN_PRONOUN_CLASSIFIER_H

#include "Generic/descriptors/PronounClassifier.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/EntityType.h"
/** Pronoun Classifier - Deterministically make a node Pronoun
 * 
 */
class EnglishPronounClassifier : public PronounClassifier {
private:
	friend class EnglishPronounClassifierFactory;

public:

	~EnglishPronounClassifier();
	/**
	 * The ClassifierTreeSearch (branchable) way to classify.
	 * The classifier uses a combination of prob models that focus on head word and
	 * functional parent head to pick a type (or OTH
	 * if no type can be found)
	 * @param currSolution The extant mentionSet
	 * @param currMention The mention to classify
	 * @param results The resultant mentionSet(s)
	 * @param max_results The largest possible size of results
	 * @param isBranching Whether we should bother with forking and branching or just overwrite
	 * @return the number of elements in results (which is used by the method calling this
	 */
	virtual int classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching);
private:
	EnglishPronounClassifier();

	EntityType _personType;

	// Heuristic to decide if a node is an EDT-relevant pronoun
	bool isReferringPronoun(const SynNode *node);

};

class EnglishPronounClassifierFactory: public PronounClassifier::Factory {
	virtual PronounClassifier *build() { return _new EnglishPronounClassifier(); }
};


#endif
