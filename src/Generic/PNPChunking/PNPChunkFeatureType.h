// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NPCHUNK_FEATURE_TYPE_H
#define NPCHUNK_FEATURE_TYPE_H
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/common/Symbol.h"


class PNPChunkFeatureType : public DTFeatureType {
public:
	static Symbol modeltype;
	PNPChunkFeatureType(Symbol name, InfoSource infoSource=InfoSource::OBSERVATION | InfoSource::PREV_TAG) : DTFeatureType(modeltype, name, infoSource) {}
};
#endif
