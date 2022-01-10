// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef ES_NAME_PARSE_CONSTRAINT_H
#define ES_NAME_PARSE_CONSTRAINT_H

#include "Generic/edt/MentionGroups/MentionGroupConstraint.h"

class SpanishNameParseClashConstraint : public MentionGroupConstraint {
public:
	SpanishNameParseClashConstraint() {}
	virtual bool violatesMergeConstraint(const MentionGroup& mg1, const MentionGroup& mg2, LinkInfoCache& cache) const;
};

#endif
