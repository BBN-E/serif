// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_HIGH_PRECISION_PERSON_NAME_MERGER_H
#define EN_HIGH_PRECISION_PERSON_NAME_MERGER_H

#include "Generic/edt/MentionGroups/MentionGroupMerger.h"
#include "Generic/common/AttributeValuePair.h"

/**
  *  Merges PER mentions when the full string of one mention matches the first or last 
  *  name of the other - e.g. "Smith" and "John Smith".
  *
  *  This merger is very similar to EnglishPersonNameMerger - the primary difference being 
  *  that the HighPrecision version only allows a merge when the value matched is unique across
  *  the document.  For example, in a document containing "Smith", "John Smith" and "Jane Smith",
  *  the HighPrecision merger won't merge "Smith" with either of the full names because the
  *  correct linking is unclear.
  */
class EnglishHighPrecisionPersonNameMerger : public MentionGroupMerger {
public:
	EnglishHighPrecisionPersonNameMerger(MentionGroupConstraint_ptr constraints);
protected:
	/** Returns true if m1 matches the first/last name of m2, or vice versa */
	bool shouldMerge(const MentionGroup& g1, const MentionGroup& g2, LinkInfoCache& cache) const;
	
	/** 
	  * Returns true if at least one pair of features from nameFeatureSet and uniqFeatureSet 
	  * matches and the feature from uniqFeatureSet doesn't occur in any other MentionGroups. 
	  */
	bool findUniqueValueMatch(const MentionGroup& g1, const MentionGroup& g2,
		                      std::set<Symbol>& nameFeatureSet, std::set<Symbol>& uniqFeatureSet,
		                      Symbol extractor, Symbol feature, LinkInfoCache& cache) const;
};

#endif
