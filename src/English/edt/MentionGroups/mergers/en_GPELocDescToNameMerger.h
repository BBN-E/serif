// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_GPE_LOC_DESC_TO_NAME_MERGER_H
#define EN_GPE_LOC_DESC_TO_NAME_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges GPE mentions with their location descriptors identified by EnglishPreLinker::preLinkGPELocDescsToNames()
  *  e.g.  "the town of Jalalabad", "Helmand province"
  */
class EnglishGPELocDescToNameMerger : public PairwiseMentionGroupMerger {
public:
	EnglishGPELocDescToNameMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Resturns true if m1 is a location descriptor for m2, or vice versa. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
