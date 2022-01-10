// Copyright (c) 2016 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEAD_WORD_CLASH_CONSTRAINT_H
#define HEAD_WORD_CLASH_CONSTRAINT_H

#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"

/**
  * Prevents merges between pairs of MentionGroups when both groups 
  * has at least one DESC or PART mention and there are no matching 
  * head words between the groups. Modeled after 
  * DescLinkFeatureFunctions::hasHeadwordClash.
  */
class HeadWordClashConstraint : public MentionGroupConstraint {
public:
	HeadWordClashConstraint() {}

	bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const;
};

#endif
