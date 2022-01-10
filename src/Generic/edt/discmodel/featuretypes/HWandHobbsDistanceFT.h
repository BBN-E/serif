// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef HW_AND_HOBBS_DISTANCE_FT_H
#define HW_AND_HOBBS_DISTANCE_FT_H

#include "Generic/common/Symbol.h"
#include "Generic/common/SymbolConstants.h"
#include "Generic/edt/discmodel/DTCorefFeatureType.h"
#include "Generic/discTagger/DTBigramIntFeature.h"
#include "Generic/discTagger/DTState.h"
#include "Generic/edt/discmodel/DTCorefObservation.h"
#include "Generic/edt/DescLinkFeatureFunctions.h"


#include "Generic/theories/Mention.h"
#include "Generic/theories/SynNode.h"


class HWandHobbsDistanceFT : public DTCorefFeatureType {
public:
	HWandHobbsDistanceFT() : DTCorefFeatureType(Symbol(L"hw-and-hobbs-distance")) {}

	virtual DTFeature *makeEmptyFeature() const {
		return _new DTBigramIntFeature(this, SymbolConstants::nullSymbol, SymbolConstants::nullSymbol, 0);
	}

	virtual int extractFeatures(const DTState &state,
								DTFeature **resultArray) const
	{
		DTCorefObservation *o = static_cast<DTCorefObservation*>(
			state.getObservation(0));

		int distance = o->getHobbsDistance();
		if (distance >= 20) distance = 20;
		else if (distance >= 10) distance = 10;

		Symbol headSym = o->getMention()->getHead()->getHeadWord();
		resultArray[0] = _new DTBigramIntFeature(this, state.getTag(), headSym, distance);
		return 1;
	}

};
#endif
