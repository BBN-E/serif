// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_STEMMED_CONNECTING_STRING_FEATURE_TYPE_H
#define AA_STEMMED_CONNECTING_STRING_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class AAStemmedConnectingStringFeatureType : public EventAAFeatureType {
public:
	AAStemmedConnectingStringFeatureType() : EventAAFeatureType(Symbol(L"stemmed-connecting-string")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(
			state.getObservation(0));

		if (o->getStemmedConnectingString().is_null())
			return 0;

		// Place Conflict.Attack attack_in_GPE
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
			o->getReducedEventType(), o->getStemmedConnectingString());
		return 1;
	}
};

#endif
