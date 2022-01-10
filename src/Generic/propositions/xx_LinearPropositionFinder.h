#ifndef XX_LINEAR_PROPOSITION_FINDER_H
#define XX_LINEAR_PROPOSITION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#include "Generic/propositions/LinearPropositionFinder.h"


class DefaultLinearPropositionFinder : public LinearPropositionFinder {
private:
	friend class DefaultLinearPropositionFinderFactory;

public:

	virtual ~DefaultLinearPropositionFinder() {}
	virtual void augmentPropositionTheory(PropositionSet *propSet,
										  const SynNode *root,
										  const MentionSet *mentionSet)
	{}

private:
	DefaultLinearPropositionFinder() : LinearPropositionFinder() {}
};

class DefaultLinearPropositionFinderFactory: public LinearPropositionFinder::Factory {
	virtual LinearPropositionFinder *build() { return _new DefaultLinearPropositionFinder(); }
};



#endif
