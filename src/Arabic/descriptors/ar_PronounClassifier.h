// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ar_PRONOUN_CLASSIFIER_H
#define ar_PRONOUN_CLASSIFIER_H

#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/descriptors/PronounClassifier.h"
#include "Generic/theories/SynNode.h"

/** Pronoun Classifier - Deterministically make a node Pronoun
 * 
 */
class SynNode;
class MentionSet;
class P1DescriptorClassifier;
class ArabicPronounClassifier : public PronounClassifier {
private:
	friend class ArabicPronounClassifierFactory;

public:
	~ArabicPronounClassifier();

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


protected:

	// COPIED UNCHANGED FROM DESC-CLASSIFIER 6/20/04
	// insert score, type into sorted list, maintaining sort
	// returns the size of the arrays after adding (either ++ or unchanged)
	int insertScore(double score, EntityType type, double scores[], EntityType types[], 
		int size, int cap);

	// returns the number of possibilities selected
	virtual EntityType classifyPronounType(
		MentionSet *currSolution, const SynNode* node);
		//EntityType types[],double scores[], int maxResults);

	P1DescriptorClassifier *_p1DescriptorClassifier;

	bool DEBUG;
	UTF8OutputStream _debugStream;

private:
	ArabicPronounClassifier();

};

class ArabicPronounClassifierFactory: public PronounClassifier::Factory {
	virtual PronounClassifier *build() { return _new ArabicPronounClassifier(); }
};

#endif
