// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PRIOR_FT_H
#define PRIOR_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/relations/discmodel/P1RelationFeatureType.h"
#include "Generic/discTagger/DTMonogramFeature.h"
#include "Generic/discTagger/DTState.h"


class PriorFT : public P1RelationFeatureType {
public:
	PriorFT() : P1RelationFeatureType(Symbol(L"prior")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTMonogramFeature(this, SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		resultArray[0] = _new DTMonogramFeature(this, state.getTag());
		return 1;
	}

};

#endif
