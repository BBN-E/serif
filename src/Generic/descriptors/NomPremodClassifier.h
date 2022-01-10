// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NOM_PREMOD_CLASSIFIER_H
#define NOM_PREMOD_CLASSIFIER_H

#include <boost/shared_ptr.hpp>

#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/descriptors/CompoundMentionFinder.h"

/** Nominal Premodifier Classifier -- modeled completely on DescriptorClassifier 
 */
class NomPremodClassifier : public MentionClassifier {
public:
	/** Create and return a new NomPremodClassifier. */
	static NomPremodClassifier *build() { return _factory()->build(); }
	/** Hook for registering new NomPremodClassifier factories */
	struct Factory { virtual NomPremodClassifier *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual void resetForNewSentence() {}
	virtual void resetForNewTheory() {}
	virtual void cleanup() {}

	virtual ~NomPremodClassifier();

	/**
	 * The ClassifierTreeSearch (branchable) way to classify.
	 * @param currSolution The extant mentionSet
	 * @param currMention The mention to classify
	 * @param results The resultant mentionSet(s)
	 * @param max_results The largest possible size of results
	 * @param isBranching Whether we should bother with forking and branching or just overwrite
	 * @return the number of elements in results (which is used by the method calling this)
	 */
	int classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching);

protected:

	NomPremodClassifier();

	// COPIED UNCHANGED FROM DESC-CLASSIFIER 6/20/04
	// insert score, type into sorted list, maintaining sort
	// returns the size of the arrays after adding (either ++ or unchanged)
	int insertScore(double score, EntityType type, double scores[], EntityType types[], 
		int size, int cap);

	virtual int classifyNomPremod (MentionSet *currSolution, const SynNode *node, EntityType types[], 
									double scores[], int maxResults) = 0;


private:
	static boost::shared_ptr<Factory> &_factory();
};

// language-specific includes determine which implementation is used
//#ifdef ENGLISH_LANGUAGE
//	#include "English/descriptors/en_NomPremodClassifier.h"
//#else
//	#include "Generic/descriptors/xx_NomPremodClassifier.h"
//#endif

#endif
