// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_N_CAND_OF_SAME_TYPE_FEATURE_TYPE_H
#define AA_N_CAND_OF_SAME_TYPE_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class AANCandOfSameTypeFeatureType : public EventAAFeatureType {
public:
	AANCandOfSameTypeFeatureType() : EventAAFeatureType(Symbol(L"n-cand-same-type")) {}

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

		Symbol number;
		int n = o->getNCandidatesOfSameType();
		if (n == 0)
			number = Symbol(L"none");
		else if (n == 1)
			number = Symbol(L"one");
		else if (n == 2)
			number = Symbol(L"two");
		else if (n == 3)
			number = Symbol(L"three");
		else
			number = Symbol(L"num>3");

		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
			o->getEventType(), o->getCandidateType(), number);
		return 1;
	}
};

#endif
