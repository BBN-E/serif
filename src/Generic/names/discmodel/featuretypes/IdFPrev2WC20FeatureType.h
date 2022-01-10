// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_PREV_2_WC20_FEATURE_TYPE_H
#define D_T_PREV_2_WC20_FEATURE_TYPE_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "names/discmodel/PIdFFeatureType.h"
#include "discTagger/DTIntFeature.h"
#include "discTagger/DTState.h"
#include "names/discmodel/TokenObservation.h"


class IdFPrev2WC20FeatureType : public PIdFFeatureType {
public:
	IdFPrev2WC20FeatureType() : PIdFFeatureType(Symbol(L"prev2-wc20"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex() <= 1){
			return 0;
		}
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()-2));
		if(o->getWordClass().c20() == 0){
			return 0;
		}

		resultArray[0] = _new DTIntFeature(this, state.getTag(), o->getWordClass().c20());
		return 1;
	}

};
#endif
