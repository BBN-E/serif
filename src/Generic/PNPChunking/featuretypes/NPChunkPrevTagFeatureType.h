// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_PREV_TAG_FEATURE_TYPE_H
#define D_T_NPCHUNK_PREV_TAG_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"


class NPChunkPrevTagFeatureType : public PNPChunkFeatureType {
public:
	NPChunkPrevTagFeatureType() : PNPChunkFeatureType(Symbol(L"prev-tag"), InfoSource::PREV_TAG) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		resultArray[0] = _new DTBigramFeature(this, state.getTag(), state.getPrevTag());
		return 1;
	}
};

#endif
