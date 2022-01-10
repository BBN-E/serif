// Copyright (c) 2014 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef OPERATOR_CLASH_CONSTRAINT_H
#define OPERATOR_CLASH_CONSTRAINT_H

#include "Generic/edt/MentionGroups/constraints/PairwiseMentionGroupConstraint.h"
#include "Generic/common/Symbol.h"

#include <set>

/**
  *  Prevents merges between MentionGroups when at least one pair of mentions across
  *  the two groups operator conflicting known accounts (email, phone, etc.).
  */
class OperatorClashConstraint : public MentionGroupConstraint { 
public:
	OperatorClashConstraint();

	/** Returns true if mg1 and mg2 have conflicting known grammatical gender. */
	virtual bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const;
protected:
	std::set<Symbol> _operatorKeys;

	/** Returns the set of all operator feature Symbols found for mentions in group. */
	std::set<Symbol> getOperatorFeatures(Symbol& operatorKey, const MentionGroup& group, LinkInfoCache& cache) const;
};

#endif
