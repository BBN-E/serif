// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef DR_P0_RIGHTNEIGHBOR_CP_POS_FEATURE_TYPE_H
#define DR_P0_RIGHTNEIGHBOR_CP_POS_FEATURE_TYPE_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/discourseRel/DiscourseRelFeatureType.h"
#include "Generic/discourseRel/DiscourseRelObservation.h"
#include "Generic/discTagger/DTQuadgramFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/names/discmodel/TokenObservation.h"


class DRP0RightNeighborPOSCPPOSFeatureType : public DiscourseRelFeatureType {
public:
	DRP0RightNeighborPOSCPPOSFeatureType() : DiscourseRelFeatureType(Symbol(L"pos,pos-rightNeighbor,pos-Parent")) {}

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
		resultArray[0] = _new DTQuadgramFeature(this, state.getTag(), o->getPOS(), o->getRightNeighborPOS(), o->getCommonParentPOSforRightNeighbor());
		return 1;
	}
};

#endif
