// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_NEXT_POS_FEATURE_TYPE_H
#define EM_NEXT_POS_FEATURE_TYPE_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "events/stat/EventModalityFeatureType.h"
#include "events/stat/EventModalityObservation.h"
#include "discTagger/DTBigramFeature.h"
#include "discTagger/DTState.h"
#include "names/discmodel/TokenObservation.h"


class EMNextPOSFeatureType : public EventModalityFeatureType {
public:
	EMNextPOSFeatureType() : EventModalityFeatureType(Symbol(L"next-pos")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventModalityObservation *o = static_cast<EventModalityObservation*>(
			state.getObservation(0));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getNextPOS());
		return 1;
	}
};

#endif
