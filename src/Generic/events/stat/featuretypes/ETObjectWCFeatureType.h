// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_OBJECT_WC_FEATURE_TYPE_H
#define ET_OBJECT_WC_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"


class ETObjectWCFeatureType : public EventTriggerFeatureType {
public:
	ETObjectWCFeatureType() : EventTriggerFeatureType(Symbol(L"object-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		Symbol object = o->getObjectOfTrigger();
		if (object.is_null())
			return 0;
		WordClusterClass wc(o->getWord());

		resultArray[0] = _new DTBigramIntFeature(this, state.getTag(), object, wc.c16());
		return 1;
	}
};

#endif
