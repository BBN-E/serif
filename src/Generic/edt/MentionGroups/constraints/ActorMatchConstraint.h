// Copyright (c) 2018 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ACTOR_MATCH_CONSTRAINT_H
#define ACTOR_MATCH_CONSTRAINT_H

#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"

/** In cases where we are doing AWAKE actor matching, we compare
  * GPE actor matches to each other to stop a merge */

class ActorMatchConstraint : public MentionGroupConstraint {
public:
	ActorMatchConstraint() { }

	bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const;
};

#endif
