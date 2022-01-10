// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HOBBS_DISTANCE_FT_H
#define HOBBS_DISTANCE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"

#include "Generic/theories/Mention.h"


class HobbsDistanceFT : public DTCorefFeatureType {
public:
	HobbsDistanceFT() : DTCorefFeatureType(Symbol(L"hobbs-distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTIntFeature(this, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		int distance = o->getHobbsDistance();
		if (distance >= 20) distance = 20;
		else if (distance >= 10) distance = 10;

		resultArray[0] = _new DTIntFeature(this, state.getTag(), distance);
		return 1;
	}

};
#endif
