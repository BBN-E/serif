// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_N1POS_FEATURE_TYPE_H
#define D_T_NPCHUNK_N1POS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"


class NPChunkN1POSFeatureType : public PNPChunkFeatureType {
public:
	NPChunkN1POSFeatureType() : PNPChunkFeatureType(Symbol(L"pos+1"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if((state.getIndex()+1) >= state.getNObservations()){
			return 0;
		}
		TokenPOSObservation *o = static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex() +1));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getPOS());
		return 1;
	}


};

#endif
