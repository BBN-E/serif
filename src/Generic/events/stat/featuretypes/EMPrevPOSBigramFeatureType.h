// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef EM_PREV_POS_BIGRAM_FEATURE_TYPE_H
#define EM_PREV_POS_BIGRAM_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventModalityFeatureType.h"
#include "Generic/events/stat/EventModalityObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"


class EMPrevPOSBigramFeatureType : public EventModalityFeatureType {
public:
	EMPrevPOSBigramFeatureType() : EventModalityFeatureType(Symbol(L"prev-pos-bigram")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventModalityObservation *o = static_cast<EventModalityObservation*>(
			state.getObservation(0));

		// option 1
		/*
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
			o->getPrevPOS(), o->getStemmedWord());

		return 1;
		*/

		// option 2
		
		resultArray[0] = _new DTTrigramFeature(this, state.getTag(),
			o->getPrevPOS(), o->getPOS());

		resultArray[1] = _new DTTrigramFeature(this, state.getTag(),
			o->getPrevPOS(), o->getStemmedWord());

		return 2;
		
	}
};

#endif
