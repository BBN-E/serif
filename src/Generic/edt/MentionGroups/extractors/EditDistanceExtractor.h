// Copyright (c) 2012 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef EDIT_DISTANCE_EXTRACTOR_H
#define EDIT_DISTANCE_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

/**
  *  Calculates an edit-distance score for a pair of name mentions.
  */
class EditDistanceExtractor : public AttributeValuePairExtractor<MentionPair> {
public:
	EditDistanceExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const MentionPair& context, LinkInfoCache& cache, const DocTheory *docTheory);

};

#endif
