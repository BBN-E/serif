// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef ET_NEXT_WC16_FEATURE_TYPE_H
#define ET_NEXT_WC16_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventTriggerFeatureType.h"
#include "Generic/events/stat/EventTriggerObservation.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"


class ETNextWC16FeatureType : public EventTriggerFeatureType {
public:
	ETNextWC16FeatureType() : EventTriggerFeatureType(Symbol(L"next-wc16")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventTriggerObservation *o = static_cast<EventTriggerObservation*>(
			state.getObservation(0));

		if (o->isLastWord())
			return 0;

		WordClusterClass wc(o->getNextWord());
		resultArray[0] = _new DTIntFeature(this, state.getTag(), wc.c16());
		return 1;
	}
};

#endif
