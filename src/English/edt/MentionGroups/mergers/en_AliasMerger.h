// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_ALIAS_MERGER_H
#define EN_ALIAS_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"
#include "Generic/common/AttributeValuePair.h"

#include <vector>

/**
  *  Merges mentions and their aliases, as identified by EnglishAliasExtractor.
  */
class EnglishAliasMerger : public PairwiseMentionGroupMerger {
public:
	EnglishAliasMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 is an alias of m2, or vice versa */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;

	/** Returns true if the value of at least one of the features is equal to ment. */
	bool findMentionFeatureMatch(const Mention *ment, std::vector<AttributeValuePair_ptr>& features) const;
};

#endif
