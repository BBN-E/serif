// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_rightNeighbor_Head_H
#define en_rightNeighbor_Head_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discourseRel/DiscourseRelFeatureType.h"
#include "Generic/discTagger/DTBigramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discourseRel/DiscourseRelObservation.h"


class EnglishRightNeighborHead : public DiscourseRelFeatureType {

public:
	EnglishRightNeighborHead() : DiscourseRelFeatureType(Symbol(L"rightNeighbor_head")) {
	
	}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramFeature(this, SymbolConstants::nullSymbol, 
			SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DiscourseRelObservation *o = static_cast<DiscourseRelObservation*>(
			state.getObservation(0));

		resultArray[0] = _new DTBigramFeature(this, state.getTag(), o->getRightNeighborHead());

		return 1;
	}

};
#endif
