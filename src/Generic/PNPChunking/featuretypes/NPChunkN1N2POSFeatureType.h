// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_N1N2POS_FEATURE_TYPE_H
#define D_T_NPCHUNK_N1N2POS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"


class NPChunkN1N2POSFeatureType : public PNPChunkFeatureType {
public:
	NPChunkN1N2POSFeatureType() : PNPChunkFeatureType(Symbol(L"pos+1,pos+2"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if((state.getIndex()+2) >= state.getNObservations()){
			return 0;
		}
		TokenPOSObservation *o_1 = static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()+1));
		TokenPOSObservation *o_2= static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex() +2));
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), o_1->getPOS(), o_2->getPOS());
		return 1;
	}


};

#endif
