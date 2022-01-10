// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DESCRIPTOR_CLASSIFIER_H
#define DESCRIPTOR_CLASSIFIER_H

#include <boost/shared_ptr.hpp>

#include "Generic/descriptors/MentionClassifier.h"
#include "Generic/theories/Entity.h"

class MentionSet;
class Mention;

/** Descriptor Classifier - Determine an EDT type for NPs that are not
 *  names or part of otherwise interesting mention structure. This class
 *  is invoked by DescriptorRecognizer
 */
class DescriptorClassifier : public MentionClassifier {
public:
	/** Create and return a new DescriptorClassifier. */
	static DescriptorClassifier *build() { return _factory()->build(); }
	/** Hook for registering new DescriptorClassifier factories */
	struct Factory { virtual DescriptorClassifier *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~DescriptorClassifier() {}
	virtual void resetForNewSentence() {}
	virtual void resetForNewTheory() {}
	virtual void cleanup() {}

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
	int classifyMention (MentionSet *currSolution, Mention *currMention, MentionSet *results[], int max_results, bool isBranching);

	// FOR BACKWARDS COMPATABILITY ONLY. AVOID USE
	EntityType classifyDescriptor(const SynNode* node, MentionSet* menSet=0);

	// insert score, type into sorted list, maintaining sort
	// returns the size of the arrays after adding (either ++ or unchanged)
	static int insertScore(double score, EntityType type, double scores[], EntityType types[], int size, int cap);
	static const int _LOG_OF_ZERO;

protected:
	// the actual probability work do-er
	// returns the number of possibilities selected
	virtual int classifyDescriptor(MentionSet *currSolution, const SynNode *node, EntityType types[], 
									double scores[], int maxResults) = 0;
	
private:
	static boost::shared_ptr<Factory> &_factory();
};

// language-specific includes determine which implementation is used
//#ifdef ENGLISH_LANGUAGE
//	#include "English/descriptors/en_DescriptorClassifier.h"
//#elif defined(CHINESE_LANGUAGE)
//	#include "Chinese/descriptors/ch_DescriptorClassifier.h"
//#elif defined(ARABIC_LANGUAGE)
//	#include "Arabic/descriptors/ar_DescriptorClassifier.h"
//#else
//	#include "descriptors/xx_DescriptorClassifier.h"
//#endif

#endif

