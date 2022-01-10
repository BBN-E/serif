// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef Discourse_Rel_FEATURE_TYPE_H
#define Discourse_Rel_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class DiscourseRelFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	DiscourseRelFeatureType(Symbol name) : DTFeatureType(modeltype, name) {}
};
#endif
