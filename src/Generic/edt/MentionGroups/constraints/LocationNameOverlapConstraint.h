// Copyright (c) 2018 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef LOCATION_NAME_OVERLAP_CONSTRAINT_H
#define LOCATION_NAME_OVERLAP_CONSTRAINT_H

#include "Generic/edt/MentionGroups/constraints/PairwiseMentionGroupConstraint.h"

#include <vector>

/**
  *  Prevents merges between GPE/LOC Mentions when the names are the 
  *  same except for certain prefixes -- "Lake Chad" vs. "Chad".
  */
class LocationNameOverlapConstraint : public PairwiseMentionGroupConstraint {
public:
	LocationNameOverlapConstraint();
protected:
	/** Returns true if m1 and m2 have conflicting recognized entity types. */
	bool violatesMergeConstraint(const Mention *m1, const Mention *m2, LinkInfoCache& cache) const;

private:
	std::vector<std::wstring> _constrainingPrefixes;
	std::vector<std::wstring> _constrainingSuffixes;

	void readConstraintFile(std::string& inputFile, std::vector<std::wstring>& constraintList);
};

#endif
