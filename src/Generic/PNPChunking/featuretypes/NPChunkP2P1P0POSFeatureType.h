// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_P2P1P0POS_FEATURE_TYPE_H
#define D_T_NPCHUNK_P2P1P0POS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"


class NPChunkP2P1P0POSFeatureType : public PNPChunkFeatureType {
public:
	NPChunkP2P1P0POSFeatureType() : PNPChunkFeatureType(Symbol(L"pos-2,pos-1,pos"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex() < 2){
			return 0;
		}
		TokenPOSObservation *o_2 = static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()-2));
		TokenPOSObservation *o_1= static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex() -1));
				TokenPOSObservation *o= static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()));
				resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), o_2->getPOS(), 
					o_1->getPOS(), o->getPOS());
		return 1;
	}


};

#endif
