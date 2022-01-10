// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_NATIONALITY_PEOPLE_MERGER_H
#define EN_NATIONALITY_PEOPLE_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges mentions that share a nationality proposition identified by EnglishPreLinker::preLinkNationalityPeople()
  *  e.g. "the American people"
  */
class EnglishNationalityPeopleMerger : public PairwiseMentionGroupMerger {
public:
	EnglishNationalityPeopleMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 refers to the people of m2, or vice versa. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
