// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_IDF_SECONDARY_DECODERS_FEATURETYPE_H
#define D_T_IDF_SECONDARY_DECODERS_FEATURETYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

#include "Generic/common/SymbolUtilities.h"


class IdFSecondaryDecoderFeatureType : public PIdFFeatureType {
public:
	IdFSecondaryDecoderFeatureType() : PIdFFeatureType(Symbol(L"idf-secondarydecoder"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		int n = o->getNDecoderFeatures();
		const Symbol* guess = o->getDecoderFeatures();
		int i = 0;
		for(i= 0; i< n; i++){
			resultArray[i] = _new DTBigramFeature(this, state.getTag(), guess[i]);
		}
		return i;
	}
};

#endif
