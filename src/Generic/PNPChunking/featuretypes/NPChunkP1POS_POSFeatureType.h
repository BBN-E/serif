// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_P1POS_POS_FEATURE_TYPE_H
#define D_T_NPCHUNK_P1POS_POS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"


class NPChunkP1POS_POSFeatureType : public PNPChunkFeatureType {
public:
	NPChunkP1POS_POSFeatureType() : PNPChunkFeatureType(Symbol(L"pos-1,pos"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex() < 1){
			return 0;
		}
		TokenPOSObservation *o = static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()));
		TokenPOSObservation *o_1= static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex() -1));
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), o_1->getPOS(), o->getPOS());
		return 1;
	}


};

#endif
