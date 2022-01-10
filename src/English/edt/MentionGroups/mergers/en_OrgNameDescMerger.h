// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_ORG_NAME_DESC_MERGER_H
#define EN_ORG_NAME_DESC_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges ORG names with their descriptors identified by EnglishPreLinker::preLinkOrgNameDescs()
  *  e.g. "the Itar-Tass news agency"
  */
class EnglishOrgNameDescMerger : public PairwiseMentionGroupMerger {
public:
	EnglishOrgNameDescMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 is a ORG descriptor for m2, or vice versa. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
