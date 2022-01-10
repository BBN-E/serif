// Copyright (c) 2016 by Raytheon BBN Technologies Corp.
// All Rights Reserved.

#ifndef HEAD_WORD_EXTRACTOR_H
#define HEAD_WORD_EXTRACTOR_H

#include "Generic/common/AttributeValuePair.h"
#include "Generic/common/AttributeValuePairExtractor.h"
#include "Generic/theories/Mention.h"

class HeadWordExtractor : public AttributeValuePairExtractor<Mention>
{
public:
	HeadWordExtractor();
	std::vector<AttributeValuePair_ptr> extractFeatures(const Mention& context, LinkInfoCache& cache, const DocTheory *docTheory);
protected:
	void validateRequiredParameters();
};


#endif
