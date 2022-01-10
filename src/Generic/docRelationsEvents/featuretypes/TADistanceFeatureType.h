// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef TA_DISTANCE_FEATURE_TYPE_H
#define TA_DISTANCE_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/docRelationsEvents/RelationTimexArgFeatureType.h"
#include "Generic/docRelationsEvents/RelationTimexArgObservation.h"
#include "Generic/discTagger/DTTrigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/theories/Proposition.h"
#include "Generic/theories/Argument.h"


class TADistanceFeatureType : public RelationTimexArgFeatureType {
public:
	TADistanceFeatureType() : RelationTimexArgFeatureType(Symbol(L"distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramFeature(this, SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol,
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationTimexArgObservation *o = static_cast<RelationTimexArgObservation*>(
			state.getObservation(0));

		Symbol distance;
		int d = o->getDistance();
		if (d == -1)
			distance = Symbol(L"undefined");
		else if (d == 0)
			distance = Symbol(L"inside-arg");
		else if (d == 1)
			distance = Symbol(L"adjacent");
		else if (d <= 5)
			distance = Symbol(L"dist<=5");
		else if (d <= 10)
			distance = Symbol(L"dist<=10");
		else
			distance = Symbol(L"dist>10");

		resultArray[0] = _new DTTrigramFeature(this, state.getTag(), 
			o->getRelationType(), distance);
		return 1;
	}
};

#endif
