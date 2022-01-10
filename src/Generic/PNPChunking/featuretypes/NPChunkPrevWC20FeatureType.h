// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_PREV_WC20_FEATURE_TYPE_H
#define D_T_NPCHUNK_PREV_WC20_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"


class NPChunkPrevWC20FeatureType : public PNPChunkFeatureType {
public:
	NPChunkPrevWC20FeatureType() : PNPChunkFeatureType(Symbol(L"prev-wc20"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex() <= 0){
			return 0;
		}
		TokenPOSObservation *o = static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()-1));

		resultArray[0] = _new DTIntFeature(this, state.getTag(), o->getWordClass().c20());
		return 1;
	}

};
#endif
