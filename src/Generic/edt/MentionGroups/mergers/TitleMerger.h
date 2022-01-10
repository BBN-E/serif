// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef TITLE_MERGER_H
#define TITLE_MERGER_H

#include "Generic/edt/MentionGroups/mergers/PairwiseMentionGroupMerger.h"

/**
  *  Merges name mentions with their corresponding title mentions.
  *
  *  Information connecting a name with a specific title must have been
  *  previously stored as an attribute in the LinkInfoCache. 
  */
class TitleMerger : public PairwiseMentionGroupMerger {
public:
	TitleMerger(MentionGroupConstraint_ptr constraints);
protected:
	bool shouldMerge(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;

	/** Returns true if m1 is a title for m2 */
	bool findTitleMatch(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
