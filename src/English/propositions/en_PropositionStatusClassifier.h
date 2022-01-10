#ifndef EN_PROPOSITION_STATUS_CLASSIFIER_H
#define EN_PROPOSITION_STATUS_CLASSIFIER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include <set>
#include "Generic/common/Symbol.h"
#include "Generic/propositions/PropositionStatusClassifier.h"
#include <boost/shared_ptr.hpp>

class SentenceTheory;
class PatternSet;
class PatternMatcher;
class Proposition;
typedef boost::shared_ptr<PatternSet> PatternSet_ptr;
typedef boost::shared_ptr<PatternMatcher> PatternMatcher_ptr;

class EnglishPropositionStatusClassifier : public PropositionStatusClassifier {
private:
	friend class EnglishPropositionStatusClassifierFactory;

public:

	virtual ~EnglishPropositionStatusClassifier() {}
	virtual void augmentPropositionTheory(DocTheory *docTheory);
private:
	EnglishPropositionStatusClassifier();

	PatternSet_ptr _propStatusPatterns;

	/** ============== New Implementation ============== */

	/** Use a pattern set (param=proposition_status_patterns) to label the status
	  * of propositions in the given sentence. */
	void augmentPropositionTheoryWithPatterns(DocTheory *docTheory, SentenceTheory *sentTheory,
		PatternMatcher_ptr propStatusPatternMatcher);

	/** Add proposition status labels that can't (easily) be added using patterns. */
	void augmentPropositionTheoryExtras(DocTheory *docTheory, SentenceTheory *sentTheory);

	/** Propagate proposition status labels that we have already assigned to 
	  * related propositions. */
	void propagatePropStatuses(DocTheory *docTheory, SentenceTheory *sentTheory);

	/** Display a pattern debug message */
	void showPatternDebugMessage(Symbol patternLabel, Symbol status, const Proposition *prop);

};

class EnglishPropositionStatusClassifierFactory: public PropositionStatusClassifier::Factory {
	virtual PropositionStatusClassifier *build() { return _new EnglishPropositionStatusClassifier(); }
};



#endif
