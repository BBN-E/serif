// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACRONYM_MERGER_H
#define ACRONYM_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"
#include "Generic/common/AttributeValuePair.h"

#include <vector>

class LinkInfoCache;
class Mention;

/** 
  *  Merges name mentions with their corresponding acronyms. 
  */
class AcronymMerger : public PairwiseMentionGroupMerger {
public:
	AcronymMerger(MentionGroupConstraint_ptr constraints); 

protected:
	/** Returns true if a name for m1 matches a possible acronym of m2, or vice versa. */
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;

	/** Returns true if at least one of the name features is a match for one of the acronym features. */
	bool findNameAcronymMatch(const std::vector<AttributeValuePair_ptr>& names, const std::vector<AttributeValuePair_ptr>& acronyms) const;
};

#endif
