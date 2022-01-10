// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NPCHUNK_NEXTWORD_FEATURE_TYPE_H
#define D_T_NPCHUNK_NEXTWORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/PNPChunking/PNPChunkFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/PNPChunking/TokenPOSObservation.h"
#include "Generic/common/InternalInconsistencyException.h"


class NPChunkNextWordFeatureType : public PNPChunkFeatureType {
public:
	NPChunkNextWordFeatureType() : PNPChunkFeatureType(Symbol(L"next-word"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex()+1 >= state.getNObservations()){
			return 0;
		}
		TokenPOSObservation *o = static_cast<TokenPOSObservation*>(
			state.getObservation(state.getIndex()+1));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getSymbol());
		return 1;
	}


};

#endif
