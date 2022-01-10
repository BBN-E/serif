// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef en_P0_RIGHTNEIGHBOR_POS_CP_POS_H
#define en_P0_RIGHTNEIGHBOR_POS_CP_POS_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discourseRel/DiscourseRelFeatureType.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/discourseRel/DiscourseRelObservation.h"


class EnglishLcW0RightNeighborPOSCPPOS : public DiscourseRelFeatureType {
public:
	EnglishLcW0RightNeighborPOSCPPOS() : DiscourseRelFeatureType(Symbol(L"word,pos-rightNeighbor,pos-Parent")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTQuadgramFeature(this, SymbolConstants::nullSymbol,
								  SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 
								  SymbolConstants::nullSymbol);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DiscourseRelObservation *o = static_cast<DiscourseRelObservation*>(
			state.getObservation(0));
		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), o->getLCWord(), o->getRightNeighborPOS(), o->getCommonParentPOSforRightNeighbor());
		return 1;
	}
};

#endif
