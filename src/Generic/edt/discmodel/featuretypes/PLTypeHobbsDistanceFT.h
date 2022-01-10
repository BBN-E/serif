// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef PL_TYPE_HOBBS_DISTANCE_FT_H
#define PL_TYPE_HOBBS_DISTANCE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"

#include "Generic/theories/Mention.h"


class PLTypeHobbsDistanceFT : public DTCorefFeatureType {
public:
	PLTypeHobbsDistanceFT() : DTCorefFeatureType(Symbol(L"type-hobbs-distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol, 
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

		resultArray[0] = _new DTBigramIntFeature(this, state.getTag(), type, distance);
		return 1;
		
	}

};
#endif
