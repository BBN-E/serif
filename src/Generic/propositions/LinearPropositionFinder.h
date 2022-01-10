// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef LINEAR_PROPOSITION_FINDER_H
#define LINEAR_PROPOSITION_FINDER_H

#include <boost/shared_ptr.hpp>


class PropositionSet;
class SynNode;
class MentionSet;

/** Applies linear (non-parse-dependant) rules to find some
  * propositions that the regular propfinder may have missed due to
  * bad parses.
  */
class LinearPropositionFinder {
public:
	/** Create and return a new LinearPropositionFinder. */
	static LinearPropositionFinder *build() { return _factory()->build(); }
	/** Hook for registering new LinearPropositionFinder factories */
	struct Factory { virtual LinearPropositionFinder *build() = 0; };
	static void setFactory(boost::shared_ptr<Factory> factory) { _factory() = factory; }

	virtual ~LinearPropositionFinder() {}

	virtual void augmentPropositionTheory(PropositionSet *propSet,
										  const SynNode *root,
										  const MentionSet *mentionSet) = 0;
protected:
	LinearPropositionFinder() {}

private:
	static boost::shared_ptr<Factory> &_factory();
};


//#if defined(ENGLISH_LANGUAGE)
//	#include "English/propositions/en_LinearPropositionFinder.h"
//#else
//	#include "Generic/propositions/xx_LinearPropositionFinder.h"
//#endif


#endif
