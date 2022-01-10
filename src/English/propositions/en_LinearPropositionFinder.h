#ifndef EN_LINEAR_PROPOSITION_FINDER_H
#define EN_LINEAR_PROPOSITION_FINDER_H

// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.


#include "Generic/common/DebugStream.h"

#include "Generic/propositions/LinearPropositionFinder.h"


class Proposition;
class SynNode;


class EnglishLinearPropositionFinder : public LinearPropositionFinder {
private:
	friend class EnglishLinearPropositionFinderFactory;

	DebugStream _debug;

public:

	virtual ~EnglishLinearPropositionFinder() {}

	//virtual void augmentPropositionTheory(PropositionSet *propSet,
	//									  const Parse *parse,
	//									  const MentionSet *mentionSet);
	
	// Currently does nothing!
	virtual void augmentPropositionTheory(PropositionSet *propSet,
										  const SynNode *node,
										  const MentionSet *mentionSet);

private:
	EnglishLinearPropositionFinder();

	static void initializeSymbols();

	void doTransitiveVerbs(PropositionSet *propSet, const SynNode *root,
						   const MentionSet *mentionSet);
	void doProximityTemps(PropositionSet *propSet, const SynNode *root,
						  const MentionSet *mentionSet);
	const SynNode *findPrecedingMentionNode(const SynNode *node,
											const MentionSet *mentionSet);
	const SynNode *findFollowingMentionNode(const SynNode *node,
											const MentionSet *mentionSet);
	Proposition *findPropositionForPredicate(const SynNode *node,
											 PropositionSet *propSet);
};

class EnglishLinearPropositionFinderFactory: public LinearPropositionFinder::Factory {
	virtual LinearPropositionFinder *build() { return _new EnglishLinearPropositionFinder(); }
};



#endif
