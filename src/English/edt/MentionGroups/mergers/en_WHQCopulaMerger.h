// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_WHQ_COPULA_MERGER_H
#define EN_WHQ_COPULA_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges mentions and relative pronouns connected by a copula.
  *  Identified by EnglishPreLinker::preLinkWHQCopulas().
  */
class EnglishWHQCopulaMerger : public PairwiseMentionGroupMerger {
public:
	EnglishWHQCopulaMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 is a relative pronoun for m2, or vice versa. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
