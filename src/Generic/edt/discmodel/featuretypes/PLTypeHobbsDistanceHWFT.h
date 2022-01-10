// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PL_TYPE_HOBBS_DISTANCE_HW_FT_H
#define PL_TYPE_HOBBS_DISTANCE_HW_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTTrigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"

#include "Generic/theories/Mention.h"


class PLTypeHobbsDistanceHWFT : public DTCorefFeatureType {
public:
	PLTypeHobbsDistanceHWFT() : DTCorefFeatureType(Symbol(L"type-hobbs-distance-hw")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTTrigramIntFeature(this, SymbolConstants::nullSymbol, 
											SymbolConstants::nullSymbol,
											SymbolConstants::nullSymbol,
											0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));
		Symbol type = o->getEntity()->getType().getName();

		int distance = o->getHobbsDistance();
		if (distance >= 20) distance = 20;
		else if (distance >= 10) distance = 10;

		Symbol hw = o->getMention()->getNode()->getHeadWord();

		resultArray[0] = _new DTTrigramIntFeature(this, state.getTag(), type, hw, distance);
		return 1;
		
	}

};
#endif
