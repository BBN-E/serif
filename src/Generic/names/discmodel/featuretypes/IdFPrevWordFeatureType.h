// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef D_T_PREVWORD_FEATURE_TYPE_H
#define D_T_PREVWORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/names/discmodel/PIdFFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/common/InternalInconsistencyException.h"


class IdFPrevWordFeatureType : public PIdFFeatureType {
public:
	IdFPrevWordFeatureType() : PIdFFeatureType(Symbol(L"prev-word"), InfoSource::OBSERVATION) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		if(state.getIndex() <= 0){
			return 0;
		}
		TokenObservation *o = static_cast<TokenObservation*>(
			state.getObservation(state.getIndex()-1));

		if (PIdFFeatureType::isVocabWord(o->getSymbol())) {
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getSymbol());
			return 1;
		} else {
			return 0;
		}
	}


};

#endif
