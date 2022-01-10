// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef AA_PROP_PARTICIPANT_FEATURE_TYPE_H
#define AA_PROP_PARTICIPANT_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/events/stat/EventAAFeatureType.h"
#include "Generic/events/stat/EventAAObservation.h"
#include "Generic/discTagger/DT6gramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Argument.h"


class AAPropParticipantFeatureType : public EventAAFeatureType {
public:
	AAPropParticipantFeatureType() : EventAAFeatureType(Symbol(L"prop-participant")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DT6gramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
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

		if (o->getCandidateRoleInCP().is_null())
			return 0;

		// both the trigger and the participant are arguments of the same proposition
		// "the arrest happened in LOC"

		int n = 0;

		// Place Justice.Arrest-Jail LOC <sub> in NULL
		resultArray[n++] = _new DT6gramFeature(this, state.getTag(), 
			o->getEventType(), o->getCandidateType(), 
			SymbolConstants::nullSymbol, 
			o->getEventRoleInCP(), o->getCandidateRoleInCP());

		// Place Justice.Arrest-Jail LOC <sub> in happen
		resultArray[n++] = _new DT6gramFeature(this, state.getTag(), 
			o->getEventType(), o->getCandidateType(), 
			o->getStemmedCPPredicate(), o->getEventRoleInCP(), o->getCandidateRoleInCP());

		// Place CUSTODY LOC <sub> in NULL
		/*if (o->getReducedEventType() != o->getEventType()) {
			resultArray[n++] = _new DT6gramFeature(this, state.getTag(), 
				o->getReducedEventType(), o->getCandidateType(), 
				SymbolConstants::nullSymbol, 
				o->getEventRoleInCP(), o->getCandidateRoleInCP());
		}*/

		return n;
	}
};

#endif
