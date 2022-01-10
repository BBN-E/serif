// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRONOUN_CLASSIFIER_H
#define PRONOUN_CLASSIFIER_H

#include <boost/shared_ptr.hpp>

#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/theories/SynNode.h"
/** Pronoun Classifier - Deterministically make a node Pronoun
 * 
 */
class PronounClassifier : public MentionClassifier {
public:
	/** Create and return a new PronounClassifier. */
	static PronounClassifier *build() { return _factory()->build(); }
	/** Hook for registering new PronounClassifier factories */
	struct Factory { virtual PronounClassifier *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~PronounClassifier(){}

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
	virtual int classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching) = 0;

protected:
	PronounClassifier(){}

private:
	static boost::shared_ptr<Factory> &_factory();
};

// language-specific includes determine which implementation is used
//#ifdef ENGLISH_LANGUAGE
//	#include "English/descriptors/en_PronounClassifier.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/descriptors/ch_PronounClassifier.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/descriptors/ar_PronounClassifier.h"
//#else
//	#include "Generic/descriptors/xx_PronounClassifier.h"
//#endif

#endif
