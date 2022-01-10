// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_CENTER_TRIGRAM_FEATURE_TYPE_H
#define ET_CENTER_TRIGRAM_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"


class ETCenterTrigramFeatureType : public EventTriggerFeatureType {
public:
	ETCenterTrigramFeatureType() : EventTriggerFeatureType(Symbol(L"center-trigram")) {}

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
			o->getPrevWord(), o->getStemmedWord(), o->getNextWord());
		return 1;
	}
};

#endif
