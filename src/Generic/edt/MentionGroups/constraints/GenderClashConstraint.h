// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef GENDER_CLASH_CONSTRAINT_H
#define GENDER_CLASH_CONSTRAINT_H

#include "Generic/edt/MentionGroups/constraints/PairwiseMentionGroupConstraint.h"
#include "Generic/common/Symbol.h"

#include <set>

/**
  *  Prevents merges between MentionGroups when at least one pair of mentions across
  *  the two groups have conflicting known grammatical genders.
  */
class GenderClashConstraint : public MentionGroupConstraint { 
public:
	GenderClashConstraint() {}

	/** Returns true if mg1 and mg2 have conflicting known grammatical gender. */
	virtual bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const;
protected:
	/** Returns the set of all gender feature Symbols found for mentions in group. */
	std::set<Symbol> getGenderFeatures(const MentionGroup& group, LinkInfoCache& cache) const;

	/** Returns true if m1 and m2 have conflicting known genders. */
	//bool violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;
};

#endif
