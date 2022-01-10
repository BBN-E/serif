// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_WC20_FEATURE_TYPE_H
#define ET_WC20_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"


class ETWC20FeatureType : public EventTriggerFeatureType {
public:
	ETWC20FeatureType() : EventTriggerFeatureType(Symbol(L"wc20")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		resultArray[0] = _new DTIntFeature(this, state.getTag(),  o->getWordClusterMC().c20());
		return 1;
	}
};

#endif
