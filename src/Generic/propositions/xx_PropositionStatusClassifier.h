#ifndef XX_PROPOSITION_STATUS_CLASSIFIER_H
#define XX_PROPOSITION_STATUS_CLASSIFIER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/propositions/PropositionStatusClassifier.h"


class DefaultPropositionStatusClassifier : public PropositionStatusClassifier {
private:
	friend class DefaultPropositionStatusClassifierFactory;

public:

	virtual ~DefaultPropositionStatusClassifier() {}
	virtual void augmentPropositionTheory(DocTheory *docTheory)
	{}

private:
	DefaultPropositionStatusClassifier() : PropositionStatusClassifier() {}

};

class DefaultPropositionStatusClassifierFactory: public PropositionStatusClassifier::Factory {
	virtual PropositionStatusClassifier *build() { return _new DefaultPropositionStatusClassifier(); }
};



#endif
