// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_CAND_HEADWORD_FEATURE_TYPE_H
#define AA_CAND_HEADWORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class AACandHeadwordFeatureType : public EventAAFeatureType {
public:
	AACandHeadwordFeatureType() : EventAAFeatureType(Symbol(L"cand-headword")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(
			state.getObservation(0));

		int d = o->getDistance();
		if (d > 10)
			return 0;

		// Place Conflict.Attack LOC
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
			o->getReducedEventType(), o->getCandidateHeadword());
		return 1;
	}
};

#endif
