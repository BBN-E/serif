// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_WC12_FEATURE_TYPE_H
#define D_T_WC12_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class IdFWC12FeatureType : public PIdFFeatureType {
public:
	IdFWC12FeatureType() : PIdFFeatureType(Symbol(L"wc12"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()));
		if(o->getWordClass().c12() == 0){
			return 0;
		}

		resultArray[0] = _new DTIntFeature(this, state.getTag(), o->getWordClass().c12());
		return 1;
	}

};

#endif
