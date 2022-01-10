// Copyright (c) 2018 by Raytheon BBN Technologies Corp.
// All Rights Reserved.


#ifndef CONFIDENT_ACTOR_MATCH_EXTRACTOR_H
#define CONFIDENT_ACTOR_MATCH_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

/** When AWAKE actor matching is on, extracts high quality actor ids
  * for a mnetion. 
  *
  * Currently just tested to work on GPEs. 
  */

class ConfidentActorMatchExtractor : public AttributeValuePairExtractor<Mention> {

public:
	ConfidentActorMatchExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);

protected:
	void validateRequiredParameters();
	bool _use_awake_info;
};

#endif
