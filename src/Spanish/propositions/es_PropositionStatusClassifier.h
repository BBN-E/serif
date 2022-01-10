#ifndef ES_PROPOSITION_STATUS_CLASSIFIER_H
#define ES_PROPOSITION_STATUS_CLASSIFIER_H

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

class SpanishPropositionStatusClassifier : public PropositionStatusClassifier {
private:
	friend class SpanishPropositionStatusClassifierFactory;

public:

	virtual ~SpanishPropositionStatusClassifier() {}
	virtual void augmentPropositionTheory(DocTheory *docTheory);
private:
	SpanishPropositionStatusClassifier();

	PatternSet_ptr _propStatusPatterns;
	bool _debugPatterns; // if true then print debug messages.

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
	void showPatternDebugMessage(Symbol patternLabel, Symbol status, Proposition *prop);

};

class SpanishPropositionStatusClassifierFactory: public PropositionStatusClassifier::Factory {
	virtual PropositionStatusClassifier *build() { return _new SpanishPropositionStatusClassifier(); }
};



#endif
