// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TA_PROP_PARTICIPANT_FEATURE_TYPE_H
#define TA_PROP_PARTICIPANT_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/RelationTimexArgFeatureType.h"
#include "Generic/docRelationsEvents/RelationTimexArgObservation.h"
#include "Generic/discTagger/DTQuintgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/PropositionSet.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/SynNode.h"
#include "Generic/theories/Argument.h"


class TAPropParticipantFeatureType : public RelationTimexArgFeatureType {
public:
	TAPropParticipantFeatureType() : RelationTimexArgFeatureType(Symbol(L"prop-participant")) {}

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
		RelationTimexArgObservation *o = static_cast<RelationTimexArgObservation*>(
			state.getObservation(0));

		if (o->getCandidateRoleInCP().is_null())
			return 0;

		// both the trigger and the participant are arguments of the same proposition


		resultArray[0] = _new DTQuintgramFeature(this, state.getTag(), 
			o->getRelationType(), 
			SymbolConstants::nullSymbol, 
			o->getRelationRoleInCP(), o->getCandidateRoleInCP());

		resultArray[1] = _new DTQuintgramFeature(this, state.getTag(), 
			o->getRelationType(), 
			o->getStemmedCPPredicate(), o->getRelationRoleInCP(), o->getCandidateRoleInCP());

		return 2;
	}
};

#endif
