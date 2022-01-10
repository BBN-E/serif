// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NEGATIVE_PROP_FT_H
#define NEGATIVE_PROP_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/relations/discmodel/RelationObservation.h"
#include "Generic/relations/discmodel/RelationPropLink.h"
#include "Generic/theories/MentionSet.h"


class NegativePropFT : public P1RelationFeatureType {
public:
	NegativePropFT() : P1RelationFeatureType(Symbol(L"negative-prop")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		RelationObservation *o = static_cast<RelationObservation*>(
			state.getObservation(0));

		RelationPropLink *link = o->getPropLink();
		if (!link->isEmpty() && link->isNegative()) {
			resultArray[0] = _new DTMonogramFeature(this, state.getTag());
			return 1;
		} else return 0;
	}

};

#endif
