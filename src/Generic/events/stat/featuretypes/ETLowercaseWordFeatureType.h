// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_LOWERCASE_WORD_FEATURE_TYPE_H
#define ET_LOWERCASE_WORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class ETLowercaseWordFeatureType : public EventTriggerFeatureType {
public:
	ETLowercaseWordFeatureType() : EventTriggerFeatureType(Symbol(L"lc-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getLCWord());
		return 1;
	}
};

#endif
