// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef OTHER_CLASSIFIER_H
#define OTHER_CLASSIFIER_H

#include "Generic/descriptors/MentionClassifier.h"

class MentionSet;
class Mention;

/** Other Classifier - Determine the EDT type (from annotation or OTH by
 *	default) for NPs that are not names or part of otherwise interesting 
 *  mention structure. This class is invoked by DescriptorClassifierTrainer
 */
class OtherClassifier : public MentionClassifier {
public:
	OtherClassifier();
	~OtherClassifier();

	// the possibility of these functions remains, but for now
	// there's nothing to reset.
	void resetForNewSentence() {}
	void resetForNewTheory() {}

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
	
};

#endif
