// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_NEXT_WORD_FEATURE_TYPE_H
#define ET_NEXT_WORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"


class ETNextWordFeatureType : public EventTriggerFeatureType {
public:
	ETNextWordFeatureType() : EventTriggerFeatureType(Symbol(L"next-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getNextWord());
		return 1;
	}
};

#endif
