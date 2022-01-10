// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_NEXT_WC8_FEATURE_TYPE_H
#define D_T_NPCHUNK_NEXT_WC8_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"


class NPChunkNextWC8FeatureType : public PNPChunkFeatureType {
public:
	NPChunkNextWC8FeatureType() : PNPChunkFeatureType(Symbol(L"next-wc8"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex()+1 >= state.getNObservations()){
			return 0;
		}
		TokenPOSObservation *o = static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()+1));

		resultArray[0] = _new DTIntFeature(this, state.getTag(), o->getWordClass().c8());
		return 1;
	}

};
#endif
