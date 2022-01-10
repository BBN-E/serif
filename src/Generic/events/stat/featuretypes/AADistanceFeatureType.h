// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_DISTANCE_FEATURE_TYPE_H
#define AA_DISTANCE_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class AADistanceFeatureType : public EventAAFeatureType {
public:
	AADistanceFeatureType() : EventAAFeatureType(Symbol(L"distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(
			state.getObservation(0));

		Symbol distance;
		int d = o->getDistance();
		if (d == 0)
			distance = Symbol(L"inside-arg");
		else if (d == 1)
			distance = Symbol(L"adjacent");
		else if (d <= 5)
			distance = Symbol(L"dist<=5");
		else if (d <= 10)
			distance = Symbol(L"dist<=10");
		else
			distance = Symbol(L"dist>10");

		// Place Conflict.Attack LOC
		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
			o->getEventType(), o->getCandidateType(), distance);

		return 1;
	}
};

#endif
