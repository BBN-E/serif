// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef NUMBER_CLASH_CONSTRAINT_H
#define NUMBER_CLASH_CONSTRAINT_H

#include "Generic/edt/MentionGroups/constraints/PairwiseMentionGroupConstraint.h"
#include "Generic/common/Symbol.h"

#include <set>

/**
  *  Prevents merges between MentionGroups when they have conflicting known grammatical number.
  */
class NumberClashConstraint : public MentionGroupConstraint {
public:
	NumberClashConstraint() {}

	/** Returns true if mg1 and mg2 have conflicting known grammatical number. */
	virtual bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const;
protected:
	/** Returns the set of all number feature Symbols found for mentions in group. */
	std::set<Symbol> getNumberFeatures(const MentionGroup& group, LinkInfoCache& cache) const;
	
	/** Returns true if m1 and m2 have conflicting known grammatical number. */
	//bool violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const {};
};

#endif
