// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_FIRST_PASS_ARG_FEATURE_TYPE_H
#define AA_FIRST_PASS_ARG_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class AAFirstPassArgFeatureType : public EventAAFeatureType {
public:
	AAFirstPassArgFeatureType() : EventAAFeatureType(Symbol(L"first-pass-arg")) {}

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

		if (state.getTag() == Symbol(L"NONE"))
			return 0;

		bool alreadyExists = o->hasArgumentWithThisRole(state.getTag());

		// Place Conflict.Attack LOC
		if (o->hasArgumentWithThisRole(state.getTag()))
			resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
				o->getEventType(), o->getCandidateType(), Symbol(L"role-already-filled"));
		else resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
				o->getEventType(), o->getCandidateType(), Symbol(L"role-not-filled"));
		return 1;
	}
};

#endif
