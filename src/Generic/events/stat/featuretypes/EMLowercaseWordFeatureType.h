// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_LOWERCASE_WORD_FEATURE_TYPE_H
#define EM_LOWERCASE_WORD_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventModalityFeatureType.h"
#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class EMLowercaseWordFeatureType : public EventModalityFeatureType {
public:
	EMLowercaseWordFeatureType() : EventModalityFeatureType(Symbol(L"lc-word")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventModalityObservation *o = static_cast<EventModalityObservation*>(
			state.getObservation(0));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getLCWord());
		return 1;
	}
};

#endif
