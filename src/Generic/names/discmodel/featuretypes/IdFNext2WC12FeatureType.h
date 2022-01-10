// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_NEXT_2_WC12_FEATURE_TYPE_H
#define D_T_NEXT_2_WC12_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class IdFNext2WC12FeatureType : public PIdFFeatureType {
public:
	IdFNext2WC12FeatureType() : PIdFFeatureType(Symbol(L"next2-wc12"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex()+2 >= state.getNObservations()){
			return 0;
		}
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()+2));
		if(o->getWordClass().c12() == 0){
			return 0;
		}
		resultArray[0] = _new DTIntFeature(this, state.getTag(), o->getWordClass().c12());
		return 1;
	}

};
#endif
