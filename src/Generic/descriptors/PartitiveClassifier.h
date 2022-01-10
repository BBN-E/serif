// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PARTITIVE_CLASSIFIER_H
#define PARTITIVE_CLASSIFIER_H

#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/descriptors/CompoundMentionFinder.h"
/** Partitive Classifier - Deterministically make a node partitive
 * 
 */
class PartitiveClassifier : public MentionClassifier {
public:
	PartitiveClassifier();
	virtual ~PartitiveClassifier();

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
	CompoundMentionFinder* _compoundMentionFinder;
};

#endif
