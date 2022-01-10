// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_WC20_FEATURE_TYPE_H
#define EM_WC20_FEATURE_TYPE_H

#include "common/Symbol.h"
#include "common/SymbolConstants.h"
#include "events/stat/EventModalityFeatureType.h"
#include "events/stat/EventModalityObservation.h"
#include "discTagger/DTIntFeature.h"
#include "discTagger/DTState.h"
#include "names/discmodel/TokenObservation.h"
#include "wordClustering/WordClusterClass.h"


class EMWC20FeatureType : public EventModalityFeatureType {
public:
	EMWC20FeatureType() : EventModalityFeatureType(Symbol(L"wc20")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventModalityObservation *o = static_cast<EventModalityObservation*>(
			state.getObservation(0));

		WordClusterClass wc(o->getWord());

		resultArray[0] = _new DTIntFeature(this, state.getTag(), wc.c20());
		return 1;
	}
};

#endif
