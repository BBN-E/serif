// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_HAS_NONASSERTED_INDICATOR_VP_H
#define EM_HAS_NONASSERTED_INDICATOR_VP_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventModalityFeatureType.h"
#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

class hasNonAssertedIndicatorVP : public EventModalityFeatureType {

public:
	hasNonAssertedIndicatorVP() : EventModalityFeatureType(Symbol(L"has-nonasserted-indicator-aboveVP")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventModalityObservation *o = static_cast<EventModalityObservation*>(
			state.getObservation(0));
		
		// option 1
		if (o->hasIndicatorsAboveVP()){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"TRUE"));
			return 1;
		}else{
			return 0;
		}

		// option 2
		/*
		if (o->hasIndicatorsAboveVP()){
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"TRUE"));
		}else{
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), Symbol(L"FALSE"));
		}
		return 1;
		*/
	}

};
#endif
