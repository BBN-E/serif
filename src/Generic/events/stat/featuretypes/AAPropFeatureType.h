// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_PROP_FEATURE_TYPE_H
#define AA_PROP_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class AAPropFeatureType : public EventAAFeatureType {
public:
	AAPropFeatureType() : EventAAFeatureType(Symbol(L"prop")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuintgramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
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
		// "he was arrested in LOC"

		int n = 0;

		// Place Justice.Arrest-Jail LOC in NULL
		resultArray[n++] = _new DTQuintgramFeature(this, state.getTag(), 
			o->getEventType(), o->getCandidateType(), 
			SymbolConstants::nullSymbol, role);

		// Place Justice.Arrest-Jail LOC in shot
		resultArray[n++] = _new DTQuintgramFeature(this, state.getTag(), 
			o->getEventType(), o->getCandidateType(), 
			o->getStemmedTrigger(), role);

		// Place CUSTODY LOC in NULL
		/*if (o->getReducedEventType() != o->getEventType()) {
			resultArray[n++] = _new DTQuintgramFeature(this, state.getTag(), 
				o->getReducedEventType(), o->getCandidateType(), 
				SymbolConstants::nullSymbol, role);
		}*/

		return n;
	}
};

#endif
