// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef SECOND_PERSON_PRONOUN_MERGER_H
#define SECOND_PERSON_PRONOUN_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges pairs of second person pronoun mentions. 
  */
class SecondPersonPronounMerger : public PairwiseMentionGroupMerger {
public:
	SecondPersonPronounMerger(MentionGroupConstraint_ptr constraints);
protected:
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
