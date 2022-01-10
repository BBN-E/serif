// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_REDUCED_TAG_FEATURE_TYPE_H
#define D_T_NPCHUNK_REDUCED_TAG_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"


class NPChunkReducedTagFeatureType : public PNPChunkFeatureType {
public:
	NPChunkReducedTagFeatureType() : PNPChunkFeatureType(Symbol(L"reduced-tag"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenPOSObservation *o =
			static_cast<TokenPOSObservation*>(state.getObservation(state.getIndex()));	

		resultArray[0] = _new DTBigramFeature(this, state.getReducedTag(), o->getSymbol());
		return 1;
	}


};

#endif
