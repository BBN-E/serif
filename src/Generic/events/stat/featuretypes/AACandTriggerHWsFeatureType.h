// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_CAND_TRIGGER_HWS_FEATURE_TYPE_H
#define AA_CAND_TRIGGER_HWS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class AACandTriggerHWsFeatureType : public EventAAFeatureType {
public:
	AACandTriggerHWsFeatureType() : EventAAFeatureType(Symbol(L"cand-trigger-headwords")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,  SymbolConstants::nullSymbol,
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
		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
			o->getReducedEventType(), o->getStemmedTrigger(), o->getCandidateHeadword());
		return 1;
	}
};

#endif
