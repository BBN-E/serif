// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_SEMI_REDUCED_TAG_FEATURE_TYPE_H
#define D_T_SEMI_REDUCED_TAG_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"


class IdFSemiReducedTagFeatureType : public PIdFFeatureType {
public:
	IdFSemiReducedTagFeatureType() : PIdFFeatureType(Symbol(L"semi-reduced-tag"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o =
			static_cast<TokenObservation*>(state.getObservation(state.getIndex()));	

		resultArray[0] = _new DTBigramFeature(this, state.getSemiReducedTag(), o->getSymbol());
		return 1;
	}


};

#endif
