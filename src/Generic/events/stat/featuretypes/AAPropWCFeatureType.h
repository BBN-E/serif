// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_PROP_WC_FEATURE_TYPE_H
#define AA_PROP_WC_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTQuadgramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class AAPropWCFeatureType : public EventAAFeatureType {
public:
	AAPropWCFeatureType() : EventAAFeatureType(Symbol(L"prop-wc")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramIntFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		EventAAObservation *o = static_cast<EventAAObservation*>(
			state.getObservation(0));

		Symbol role = o->getCandidateRoleInTriggerProp();
		if (role.is_null())
			return 0;
		
		// the participant is an argument of the trigger proposition
		// "he was shot in LOC"

		// Place Conflict.Attack LOC in 908232
		resultArray[0] = _new DTQuadgramIntFeature(this, state.getTag(), 
			o->getEventType(), o->getCandidateType(), role,
			o->getTriggerWC().c12());
		resultArray[1] = _new DTQuadgramIntFeature(this, state.getTag(), 
			o->getEventType(), o->getCandidateType(), role,
			o->getTriggerWC().c16());

		return 2;
	}
};

#endif
