// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_P1P0N1POS_FEATURE_TYPE_H
#define D_T_NPCHUNK_P1P0N1POS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"


class NPChunkP1P0N1POSFeatureType : public PNPChunkFeatureType {
public:
	NPChunkP1P0N1POSFeatureType() : PNPChunkFeatureType(Symbol(L"pos-1,pos,pos+1"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol, 
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol,
									SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if((state.getIndex() < 1) || ((state.getIndex()+1) >=state.getNObservations())){
			return 0;
		}
		TokenPOSObservation *o_2 = static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()-1));
		TokenPOSObservation *o_1= static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()+1));
				TokenPOSObservation *o= static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()));
				resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), o_2->getPOS(), 
					o->getPOS(), o_1->getPOS());
		return 1;
	}


};

#endif
