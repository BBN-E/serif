// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_PREV_TRIGRAM_FEATURE_TYPE_H
#define ET_PREV_TRIGRAM_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"


class ETPrevTrigramFeatureType : public EventTriggerFeatureType {
public:
	ETPrevTrigramFeatureType() : EventTriggerFeatureType(Symbol(L"prev-trigram")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(),
			o->getSecondPrevWord(), o->getPrevWord(), o->getStemmedWord());
		return 1;
	}
};

#endif
