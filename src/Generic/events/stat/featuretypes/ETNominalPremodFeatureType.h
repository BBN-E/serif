// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_NOMINAL_PREMOD_FEATURE_TYPE_H
#define ET_NOMINAL_PREMOD_FEATURE_TYPE_H

#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class ETNominalPremodFeatureType : public EventTriggerFeatureType {
public:
	ETNominalPremodFeatureType() : EventTriggerFeatureType(Symbol(L"premod")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
										SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		if (o->isNominalPremod()) {
			resultArray[0] = _new DTBigramFeature(this, state.getTag(), 
										o->getLCWord());
			return 1;
		}
		return 0;
	}
};

#endif
