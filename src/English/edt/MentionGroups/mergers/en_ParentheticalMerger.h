// Copyright (c) 2014 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_PARENTHETICAL_MERGER_H
#define EN_PARENTHETICAL_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges mentions connected by parentheticals or their contents.
  */
class EnglishParentheticalMerger : public PairwiseMentionGroupMerger {
public:
	EnglishParentheticalMerger(MentionGroupConstraint_ptr constraints);
protected:
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
