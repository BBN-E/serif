// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NEXT_WC16_FEATURE_TYPE_H
#define D_T_NEXT_WC16_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class IdFNextWC16FeatureType : public PIdFFeatureType {
public:
	IdFNextWC16FeatureType() : PIdFFeatureType(Symbol(L"next-wc16"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex()+1 >= state.getNObservations()){
			return 0;
		}
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()+1));
		if(o->getWordClass().c16() == 0){
			return 0;
		}

		resultArray[0] = _new DTIntFeature(this, state.getTag(), o->getWordClass().c16());
		return 1;
	}

};
#endif
