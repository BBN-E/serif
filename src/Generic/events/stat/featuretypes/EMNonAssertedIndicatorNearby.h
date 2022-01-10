// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_NONASSERTED_INDICATOR_NEARBY_H
#define EM_NONASSERTED_INDICATOR_NEARBY_H

#include "Generic/common/SessionLogger.h"
#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventModalityFeatureType.h"
#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTFeatureType.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"

class nonAssertedIndicatorNearby : public EventModalityFeatureType {

public:
	nonAssertedIndicatorNearby() : EventModalityFeatureType(Symbol(L"nonasserted-indicator-nearby")) {}

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
		Symbol *IndicatorsNearby = o->getIndicatorsNearby();
		int NofIndicators = o->getNIndicatorsNearby();
		if (NofIndicators > DTFeatureType::MAX_FEATURES_PER_EXTRACTION){
			SessionLogger::warn("DT_feature_limit") <<"EMNonAssertedIndicatorNearby discarding features beyond MAX_FEATURES_PER_EXTRACTION\n";
			NofIndicators = DTFeatureType::MAX_FEATURES_PER_EXTRACTION;
		}
		for (int i=0; i < NofIndicators; i++){
			resultArray[i] = _new DTBigramFeature(this, state.getTag(), IndicatorsNearby[i]);
		}
		return NofIndicators;
		
	}

};
#endif
