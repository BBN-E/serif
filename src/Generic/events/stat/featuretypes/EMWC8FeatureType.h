// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_WC8_FEATURE_TYPE_H
#define EM_WC8_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventModalityFeatureType.h"
#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"


class EMWC8FeatureType : public EventModalityFeatureType {
public:
	EMWC8FeatureType() : EventModalityFeatureType(Symbol(L"wc8")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventModalityObservation *o = static_cast<EventModalityObservation*>(
			state.getObservation(0));

		WordClusterClass wc(o->getWord());

		resultArray[0] = _new DTIntFeature(this, state.getTag(), wc.c8());
		return 1;
	}
};

#endif
