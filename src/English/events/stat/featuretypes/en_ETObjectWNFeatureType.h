// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EN_ET_OBJECT_WN_FEATURE_TYPE_H
#define EN_ET_OBJECT_WN_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"


class EnglishETObjectWNFeatureType : public EventTriggerFeatureType {
public:
	EnglishETObjectWNFeatureType() : EventTriggerFeatureType(Symbol(L"object-wordnet")) {}

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

		int offset = 1;
		int nfeatures = 0;
		while (true) {
			int value = o->getReversedNthOffset(offset);
			if (value != -1) {
				resultArray[nfeatures++] = _new DTBigramIntFeature(this, 
					state.getTag(), object, value);
			} else break;
			offset += 3;
		}
		return nfeatures;
	}
};

#endif
