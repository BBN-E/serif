// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_ALL_IDFWORD_FEATURE_TYPE_H
#define D_T_ALL_IDFWORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class IdFAllIdFWordFeatFeatureType : public PIdFFeatureType {
public:
	IdFAllIdFWordFeatFeatureType() : PIdFFeatureType(Symbol(L"all-idfwordfeat"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));

		for (int i = 0; i < o->getNIDFWordFeatures(); i++) {
			resultArray[i] = _new DTBigramFeature(this, state.getTag(), o->getNthIDFWordFeature(i));
		}
		return o->getNIDFWordFeatures();
	}

};

#endif
