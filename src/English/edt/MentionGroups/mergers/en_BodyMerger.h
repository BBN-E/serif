// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_BODY_MERGER_H
#define EN_BODY_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges mentions that share a "body" proposition as identified by EnglishPreLinker::preLinkBody()
  *  e.g. "Smith's body" or "the body of Smith".
  */
class EnglishBodyMerger : public PairwiseMentionGroupMerger {
public:
	EnglishBodyMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 refers to the body of m2, or vice versa. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
