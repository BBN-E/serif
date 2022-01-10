// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TA_PROP_FEATURE_TYPE_H
#define TA_PROP_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/RelationTimexArgFeatureType.h"
#include "Generic/docRelationsEvents/RelationTimexArgObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class TAPropFeatureType : public RelationTimexArgFeatureType {
public:
	TAPropFeatureType() : RelationTimexArgFeatureType(Symbol(L"prop")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationTimexArgObservation *o = static_cast<RelationTimexArgObservation*>(
			state.getObservation(0));

		Symbol role = o->getCandidateRoleInPredicateProp();
		if (role.is_null())
			return 0;
		
		// the time is an argument of the predicate proposition
		// "he went to Florida in May"

		// Time-Within PHYS.Located in NULL
		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), 
			o->getRelationType(), SymbolConstants::nullSymbol, role);

		// Time-Within PHYS.Located in went
		resultArray[1] = _new DTQuadgramFeature(this, state.getTag(), 
			o->getRelationType(), o->getStemmedPredicate(), role);

		return 2;
	}
};

#endif
