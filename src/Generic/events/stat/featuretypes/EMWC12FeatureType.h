// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_WC12_FEATURE_TYPE_H
#define EM_WC12_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventModalityFeatureType.h"
#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"
#include "Generic/wordClustering/WordClusterClass.h"


class EMWC12FeatureType : public EventModalityFeatureType {
public:
	EMWC12FeatureType() : EventModalityFeatureType(Symbol(L"wc12")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, -1);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventModalityObservation *o = static_cast<EventModalityObservation*>(
			state.getObservation(0));

		WordClusterClass wc(o->getWord());

		resultArray[0] = _new DTIntFeature(this, state.getTag(), wc.c12());
		return 1;
	}
};

#endif
