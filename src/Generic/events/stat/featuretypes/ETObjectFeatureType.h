// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_OBJECT_FEATURE_TYPE_H
#define ET_OBJECT_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"


class ETObjectFeatureType : public EventTriggerFeatureType {
public:
	ETObjectFeatureType() : EventTriggerFeatureType(Symbol(L"object")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		Symbol object = o->getObjectOfTrigger();
		if (object.is_null())
			return 0;
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
			o->getStemmedWord(), object);
		return 1;
	}
};

#endif
