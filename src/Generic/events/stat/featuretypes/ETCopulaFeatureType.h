// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_COPULA_FEATURE_TYPE_H
#define ET_COPULA_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"


class ETCopulaFeatureType : public EventTriggerFeatureType {
public:
	ETCopulaFeatureType() : EventTriggerFeatureType(Symbol(L"copula")) {}

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

		if (!o->isCopula())
			return 0;

		Symbol subject = o->getSubjectOfTrigger();
		Symbol object = o->getObjectOfTrigger();
		if (subject.is_null() || object.is_null())
			return 0;
		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), subject,
			o->getLCWord(), object);
		resultArray[1] = _new DTQuadgramFeature(this, state.getTag(), Symbol(L":NULL"),
			o->getLCWord(), object);
		return 2;
	}
};

#endif
